#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mysql_util.h"

#include <my_global.h>
#include <mysql.h>

#include "db.h"
#include "util.h"

static const char* requestSources[] = {"buttons", "schedule", "android", "web"};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                   WRAPPER AROUND mysql_util.c/h                                         //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void closeDatabase() {
    closeMySQLDatabase();
}

void initalizeDatabase() {
    initalizeMySQLDatabase();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        THERMOSTAT FUNCTIONS                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void insertTemperatureHumidity(TEMP_HUM *th) {
    char *insertSQL = "INSERT INTO temp_hum (th_temperature, th_humidity)"
                    " VALUES (?, ?)";
    const int PARAM_COUNT = 2;

    MYSQL_BIND *param = calloc(PARAM_COUNT, sizeof(MYSQL_BIND));
    addBindParameterFloat(&param[0], &(th->temperature));
    addBindParameterFloat(&param[1], &(th->humidity));

    initializeAndExecuteStatement(insertSQL, strlen(insertSQL), param, NULL);

    free(param);
}

void insertBoilerActivated(int on) {
    char *insertSQL = "INSERT INTO boiler_activated (ba_set_to_on)"
                    " VALUES (?)";
    const int PARAM_COUNT = 1;

    signed char bit = (signed char)on;

    MYSQL_BIND *param = calloc(PARAM_COUNT, sizeof(MYSQL_BIND));
    addBindParameterTiny(&param[0], &bit);

    initializeAndExecuteStatement(insertSQL, strlen(insertSQL), param, NULL);

    free(param);
}

/*
The 'requestSource' parameter should be one the REQUEST_SOURCE_XXX defines.
*/
void insertTemperatureRequest(SCHEDULE *schedule, int requestSource) {
    char *insertSQL = "INSERT INTO temp_request (tr_timestamp, tr_temperature, tr_source)"
                    " VALUES (?, ?, ?)";
    const int PARAM_COUNT = 3;

    MYSQL_TIME timestamp;
    copyStructTmToMySQLTime(&(schedule->from), &timestamp);
    const char *source = requestSources[requestSource];
    unsigned long sourceLength = strlen(source);

    MYSQL_BIND *param = calloc(PARAM_COUNT, sizeof(MYSQL_BIND));
    addBindParameterTimestamp(&param[0], &timestamp);
    addBindParameterFloat(&param[1], &(schedule->requestedTemperature));
    addBindParameterString(&param[2], source, &sourceLength);

    initializeAndExecuteStatement(insertSQL, strlen(insertSQL), param, NULL);

    free(param);
}

void insertTemperatureRequestButtons(float requestedTemperature) {
    char *insertSQL = "INSERT INTO temp_request (tr_temperature)"
                    " VALUES (?)";
    const int PARAM_COUNT = 1;

    MYSQL_BIND *param = calloc(PARAM_COUNT, sizeof(MYSQL_BIND));
    addBindParameterFloat(&param[0], &requestedTemperature);

    initializeAndExecuteStatement(insertSQL, strlen(insertSQL), param, NULL);

    free(param);
}

float checkAndApplyScheduleActivation() {
    struct tm localDateTime = getLocalTime();
    SCHEDULE schedule = fetchSchedule(&localDateTime);
    SCHEDULE request = fetchTemperatureRequest(&localDateTime);
    if (request.requestedTemperature == INVALID_FETCHED_TEMPERATURE || isBefore(&(request.from), &(schedule.from))) {
        // Create a new temp_request for the current active schedule:
        insertTemperatureRequest(&schedule, REQUEST_SOURCE_SCHEDULE);

        return schedule.requestedTemperature;
    }
    else {
        return request.requestedTemperature;
    }
}

SCHEDULE fetchTemperatureRequest(struct tm *localTime) {
    SCHEDULE schedule;
    schedule.requestedTemperature = INVALID_FETCHED_TEMPERATURE;

    char *selectSQL = "SELECT tr_temperature, tr_timestamp"
                    " FROM temp_request"
                    " WHERE tr_timestamp <= ? AND tr_timestamp > DATE_SUB(?, INTERVAL 1 DAY)"
                    " ORDER BY tr_timestamp DESC"
                    " LIMIT 1";
    const int PARAM_COUNT = 2;
    const int COLUMN_COUNT = 2;

    MYSQL_TIME rightNow;
    copyStructTmToMySQLTime(localTime, &rightNow);
    float fetchedTemperature;
    MYSQL_TIME fetchedDateTime;
    my_bool is_null[COLUMN_COUNT];

    MYSQL_BIND *param = calloc(PARAM_COUNT, sizeof(MYSQL_BIND));
    addBindParameterTimestamp(&param[0], &rightNow);
    addBindParameterTimestamp(&param[1], &rightNow);

    MYSQL_BIND *result = calloc(COLUMN_COUNT, sizeof(MYSQL_BIND));
    addBindResultFloat(&result[0], &is_null[0], &fetchedTemperature);
    addBindResultTimestamp(&result[1], &is_null[1], &fetchedDateTime);

    int errorCode = initializeAndExecuteStatement(selectSQL, strlen(selectSQL), param, result);

    if (!errorCode && !containsNullColumn(result, COLUMN_COUNT)) {
        schedule.requestedTemperature = fetchedTemperature;
        copyMySQLTimeToStructTm(&fetchedDateTime, &(schedule.from));
    }

    free(param);
    free(result);

    return schedule;
}

SCHEDULE fetchSchedule(struct tm *localTime) {
    SCHEDULE schedule;
    schedule = fetchOneTimeSchedule(localTime);

    if (schedule.requestedTemperature == INVALID_FETCHED_TEMPERATURE) {
        long long int schedId = fetchFixedScheduleParent(localTime);

        if (schedId != INVALID_ID) {
            schedule = fetchFixedSchedule(schedId, localTime);
        }
    }

    return schedule;
}

static long long int fetchFixedScheduleParentStartOfYear() {
    long long int scheduleId = INVALID_ID;

    char *selectSQL = "SELECT ts_id"
                    " FROM temp_schedule"
                    " WHERE ts_from = (SELECT MAX(ts_from) FROM temp_schedule)";
    const int COLUMN_COUNT = 1;

    long long int fetchedScheduleId;
    my_bool is_null[COLUMN_COUNT];

    MYSQL_BIND *result = calloc(COLUMN_COUNT, sizeof(MYSQL_BIND));
    addBindResultLongLong(&result[0], &is_null[0], &fetchedScheduleId);

    int errorCode = initializeAndExecuteStatement(selectSQL, strlen(selectSQL), NULL, result);

    if (!errorCode && !containsNullColumn(result, COLUMN_COUNT)) {
        scheduleId = fetchedScheduleId;
    }

    free(result);

    return scheduleId;
}

long long int fetchFixedScheduleParent(struct tm *localTime) {
    long long int scheduleId = INVALID_ID;

    char *selectSQL = "SELECT ts_id"
                    " FROM temp_schedule"
                    " WHERE ts_from <= DATE_FORMAT(?, '0004-%m-%d')"
                    " ORDER BY ts_from DESC"
                    " LIMIT 1";
    const int PARAM_COUNT = 1;
    const int COLUMN_COUNT = 1;

    MYSQL_TIME rightNow;
    copyStructTmToMySQLTime(localTime, &rightNow);
    long long int fetchedScheduleId;
    my_bool is_null[COLUMN_COUNT];

    MYSQL_BIND *param = calloc(PARAM_COUNT, sizeof(MYSQL_BIND));
    addBindParameterTimestamp(&param[0], &rightNow);

    MYSQL_BIND *result = calloc(COLUMN_COUNT, sizeof(MYSQL_BIND));
    addBindResultLongLong(&result[0], &is_null[0], &fetchedScheduleId);

    int errorCode = initializeAndExecuteStatement(selectSQL, strlen(selectSQL), param, result);

    if (!errorCode && !containsNullColumn(result, COLUMN_COUNT)) {
        scheduleId = fetchedScheduleId;
    }

    free(param);
    free(result);

    if (scheduleId == INVALID_ID) {
        scheduleId = fetchFixedScheduleParentStartOfYear();
    }

    return scheduleId;
}

static SCHEDULE fetchFixedScheduleEndOfWeek(long long int scheduleId, struct tm *localTime) {
    SCHEDULE schedule;
    schedule.requestedTemperature = INVALID_FETCHED_TEMPERATURE;

    char *selectSQL = "SELECT tsd_temperature, tsd_time, tsd_day_of_week"
                    " FROM temp_schedule_detail"
                    " WHERE tsd_ts_id = ?"
                    " ORDER BY tsd_day_of_week DESC, tsd_time DESC"
                    " LIMIT 1";
    const int PARAM_COUNT = 1;
    const int COLUMN_COUNT = 3;

    float fetchedTemperature;
    MYSQL_TIME fetchedTime;
    signed char fetchedDayOfWeek;
    my_bool is_null[COLUMN_COUNT];

    MYSQL_BIND *param = calloc(PARAM_COUNT, sizeof(MYSQL_BIND));
    addBindParameterLongLong(&param[0], &scheduleId);

    MYSQL_BIND *result = calloc(COLUMN_COUNT, sizeof(MYSQL_BIND));
    addBindResultFloat(&result[0], &is_null[0], &fetchedTemperature);
    addBindResultTimestamp(&result[1], &is_null[1], &fetchedTime);
    addBindResultTiny(&result[2], &is_null[2], &fetchedDayOfWeek);

    int errorCode = initializeAndExecuteStatement(selectSQL, strlen(selectSQL), param, result);

    if (!errorCode && !containsNullColumn(result, COLUMN_COUNT)) {
        schedule.requestedTemperature = fetchedTemperature;
        convertMySQLTimeToDateTime(&fetchedTime, fetchedDayOfWeek, &(schedule.from), localTime);
    }

    free(param);
    free(result);

    return schedule;
}

SCHEDULE fetchFixedSchedule(long long int scheduleId, struct tm *localTime) {
    SCHEDULE schedule;
    schedule.requestedTemperature = INVALID_FETCHED_TEMPERATURE;

    char *selectSQL = "SELECT tsd_temperature, tsd_time, tsd_day_of_week"
                    " FROM temp_schedule_detail"
                    " WHERE tsd_ts_id = ?"
                    " AND ((tsd_day_of_week = WEEKDAY(?) AND tsd_time <= TIME(?))"
                    " OR tsd_day_of_week < WEEKDAY(?))"
                    " ORDER BY tsd_day_of_week DESC, tsd_time DESC"
                    " LIMIT 1";
    const int PARAM_COUNT = 4;
    const int COLUMN_COUNT = 3;

    MYSQL_TIME rightNow;
    copyStructTmToMySQLTime(localTime, &rightNow);
    float fetchedTemperature;
    MYSQL_TIME fetchedTime;
    signed char fetchedDayOfWeek;
    my_bool is_null[COLUMN_COUNT];

    MYSQL_BIND *param = calloc(PARAM_COUNT, sizeof(MYSQL_BIND));
    addBindParameterLongLong(&param[0], &scheduleId);
    addBindParameterTimestamp(&param[1], &rightNow);
    addBindParameterTimestamp(&param[2], &rightNow);
    addBindParameterTimestamp(&param[3], &rightNow);

    MYSQL_BIND *result = calloc(COLUMN_COUNT, sizeof(MYSQL_BIND));
    addBindResultFloat(&result[0], &is_null[0], &fetchedTemperature);
    addBindResultTimestamp(&result[1], &is_null[1], &fetchedTime);
    addBindResultTiny(&result[2], &is_null[2], &fetchedDayOfWeek);

    int errorCode = initializeAndExecuteStatement(selectSQL, strlen(selectSQL), param, result);

    if (!errorCode && !containsNullColumn(result, COLUMN_COUNT)) {
        schedule.requestedTemperature = fetchedTemperature;
        convertMySQLTimeToDateTime(&fetchedTime, fetchedDayOfWeek, &(schedule.from), localTime);
    }
    else if (errorCode <= 0) {
        // No result or NULL in the result
        schedule = fetchFixedScheduleEndOfWeek(scheduleId, localTime);
    }

    free(param);
    free(result);

    return schedule;
}

SCHEDULE fetchOneTimeSchedule(struct tm *localTime) {
    SCHEDULE schedule;
    schedule.requestedTemperature = INVALID_FETCHED_TEMPERATURE;

    char *selectSQL = "SELECT ots_temperature, ots_from_timestamp"
                    " FROM one_time_schedule"
                    " WHERE ? >= ots_from_timestamp"
                    " AND ? < ots_until_timestamp";
    const int PARAM_COUNT = 2;
    const int COLUMN_COUNT = 2;

    MYSQL_TIME rightNow;
    copyStructTmToMySQLTime(localTime, &rightNow);
    float fetchedTemperature;
    MYSQL_TIME fetchedDateTime;
    my_bool is_null[COLUMN_COUNT];

    MYSQL_BIND *param = calloc(PARAM_COUNT, sizeof(MYSQL_BIND));
    addBindParameterTimestamp(&param[0], &rightNow);
    addBindParameterTimestamp(&param[1], &rightNow);

    MYSQL_BIND *result = calloc(COLUMN_COUNT, sizeof(MYSQL_BIND));
    addBindResultFloat(&result[0], &is_null[0], &fetchedTemperature);
    addBindResultTimestamp(&result[1], &is_null[1], &fetchedDateTime);

    int errorCode = initializeAndExecuteStatement(selectSQL, strlen(selectSQL), param, result);

    if (!errorCode && !containsNullColumn(result, COLUMN_COUNT)) {
        schedule.requestedTemperature = fetchedTemperature;
        copyMySQLTimeToStructTm(&fetchedDateTime, &(schedule.from));
    }

    free(param);
    free(result);

    return schedule;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                            TEST FUNCTIONS                                               //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void deleteTempHumFrom1900() {
    char deleteSQL[] = "DELETE FROM temp_hum WHERE th_timestamp = \"1900-01-01\"";
    printf("Executing query: %s\n", deleteSQL);
    if (mysql_query(con, deleteSQL)) {
        reportMySQLError();
    }
}

void insertTempHumFrom1900() {
    char insertSQL[] = "INSERT INTO temp_hum (th_timestamp, th_temperature, th_humidity) VALUES (\"1900-01-01\", 10.00, 20.00)";
    printf("Executing query: %s\n", insertSQL);
    if (mysql_query(con, insertSQL)) {
        reportMySQLError();
    }
}

void printCountTempHumFrom1900() {
    if (mysql_query(con, "SELECT COUNT(*) FROM temp_hum WHERE th_timestamp = \"1900-01-01\"")) {
        reportMySQLError();
    }
    MYSQL_RES *result = mysql_store_result(con);
    MYSQL_ROW row = mysql_fetch_row(result);
    printf("Number of records from 1900-01-01: %s\n", row[0]);
    mysql_free_result(result);
}

void printTempHumFrom1900() {
    printf("Records from 1900-01-01:\n");
    if (mysql_query(con, "SELECT * FROM temp_hum WHERE th_timestamp = \"1900-01-01\"")) {
        reportMySQLError();
    }
    MYSQL_RES *result = mysql_store_result(con);
    int num_fields = mysql_num_fields(result);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        for(int i = 0; i < num_fields; i++) {
            printf("%20s ", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }
    mysql_free_result(result);
}
