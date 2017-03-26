#include <time.h>

#include "mysql_util.h"

#include <my_global.h>
#include <mysql.h>

#include "ini_file.h"
#include "util.h"

void closeMySQLDatabase() {
    mysql_close(con);
}

void initalizeMySQLDatabase() {
    con = mysql_init(NULL);

    if (con == NULL) {
        reportMySQLError();
    }

    printf("Connecting to %s:%d, database %s, user %s.\n", mysqlHost, mysqlPort, mysqlDatabase, mysqlUser);
    fflush(stdout);
    if (mysql_real_connect(con, mysqlHost, mysqlUser, mysqlPasswd, mysqlDatabase, mysqlPort, mysqlSocket, 0) == NULL) {
        reportMySQLError();
    }
}

void reportMySQLError() {
    fprintf(stderr, "%s\n", mysql_error(con));
    fflush(stderr);
}

void reportMySQLStatementError(MYSQL_STMT *stmt) {
    fprintf(stderr, "%s\n", mysql_stmt_error(stmt));
    fflush(stderr);
}

void reportGenericError(const char *errorMessage) {
    fprintf(stderr, "%s\n", errorMessage);
    fflush(stderr);
}

int initializeAndExecuteStatement(char * sql, size_t sqlLength, MYSQL_BIND *param, MYSQL_BIND *result) {
    int returnCode = 0;
    MYSQL_STMT *stmt = mysql_stmt_init(con);

    if (stmt == NULL) {
        reportGenericError("Could not initialize MySQL statement handler.");
        returnCode = 1;
    }
    if (mysql_stmt_prepare(stmt, sql, sqlLength) != 0) {
        reportMySQLStatementError(stmt);
        returnCode = 2;
    }
    if (param != NULL && mysql_stmt_bind_param(stmt, param) != 0) {
        reportMySQLStatementError(stmt);
        returnCode = 3;
    }
    if (result != NULL && mysql_stmt_bind_result(stmt, result) != 0) {
        reportMySQLStatementError(stmt);
        returnCode = 4;
    }
    if (mysql_stmt_execute(stmt) != 0) {
        reportMySQLStatementError(stmt);
        returnCode = 5;
    }
    if (result != NULL && mysql_stmt_store_result(stmt) != 0) {
        reportMySQLStatementError(stmt);
        returnCode = 6;
    }

    if (result != NULL) {
        int fetchResult = mysql_stmt_fetch(stmt);
        if (fetchResult == 1){
            reportMySQLStatementError(stmt);
            returnCode = 7;
        }
        else if (fetchResult == MYSQL_NO_DATA) {
            returnCode = -1;
        }
    }

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);

    return returnCode;
}

int containsNullColumn(MYSQL_BIND *result, int columnCount) {
    for (int i = 0; i < columnCount; i++) {
        if (*(result[i].is_null)) {
            return TRUE;
        }
    }
    return FALSE;
}

void addBindParameterLongLong(MYSQL_BIND *param, long long int *value) {
    param->buffer_type = MYSQL_TYPE_LONGLONG;
    param->buffer      = (void *)value;

    //param->is_unsigned   = 0;
    //param->is_null       = 0;
    //param->length        = 0;
    //param->buffer_length = 0;
}

void addBindParameterTimestamp(MYSQL_BIND *param, MYSQL_TIME *value) {
    param->buffer_type = MYSQL_TYPE_TIMESTAMP;
    param->buffer      = (void *)value;
}

void addBindParameterFloat(MYSQL_BIND *param, float *value) {
    param->buffer_type = MYSQL_TYPE_FLOAT;
    param->buffer      = (void *)value;
}

void addBindParameterString(MYSQL_BIND *param, const char *value, unsigned long *length) {
    param->buffer_type = MYSQL_TYPE_STRING;
    param->buffer      = (void *)value;
    param->length      = length;
}

void addBindParameterTiny(MYSQL_BIND *param, signed char *value) {
    param->buffer_type = MYSQL_TYPE_TINY;
    param->buffer      = (void *)value;
}

void addBindResultLongLong(MYSQL_BIND *result, my_bool *isNull, long long int *value) {
    result->buffer_type = MYSQL_TYPE_LONGLONG;
    result->buffer      = (void *)value;
    result->is_null     = isNull;
}

void addBindResultFloat(MYSQL_BIND *result, my_bool *isNull, float *value) {
    result->buffer_type = MYSQL_TYPE_FLOAT;
    result->buffer      = (void *)value;
    result->is_null     = isNull;
}

void addBindResultTimestamp(MYSQL_BIND *result, my_bool *isNull, MYSQL_TIME *value) {
    result->buffer_type = MYSQL_TYPE_TIMESTAMP;
    result->buffer      = (void *)value;
    result->is_null     = isNull;
}

void addBindResultTiny(MYSQL_BIND *result, my_bool *isNull, signed char *value) {
    result->buffer_type = MYSQL_TYPE_TINY;
    result->buffer      = (void *)value;
    result->is_null     = isNull;
}

/*
The combination of sourceTime (hour,minute,second) and sourceDayOfWeek(0=Monday, ..., 6=Sunday)
is converted into 'destinationDateTime' (DATE+TIME), relative to the current date/time ('localDateTime').
*/
void convertMySQLTimeToDateTime(MYSQL_TIME *sourceTime, signed char sourceDayOfWeek, struct tm *destinationDateTime, struct tm *localDateTime) {
    // Convert (0=sun,...,6=sat) to (0=mon,...,6=sun)
    int currentDayOfWeek = (localDateTime->tm_wday ? localDateTime->tm_wday : 7) - 1;

    // Copy the local date time:
    *destinationDateTime = *localDateTime;

    // Overwrite the time fields:
    destinationDateTime->tm_hour = sourceTime->hour;
    destinationDateTime->tm_min = sourceTime->minute;
    destinationDateTime->tm_sec = sourceTime->second;

    // ex 1: thu(3) - fri(4) => -1 OK
    // ex 2: sun(6) - mon(0) => 6 ==> -1 OK
    // ex 3: fri(4) - mon(0) => 4 ==> -3 OK
    int dayDifference = sourceDayOfWeek - currentDayOfWeek;
    if (dayDifference > 0) {
        dayDifference -= 7;
    }
    // This next function calls 'mktime', which is necessary after setting the fields of a 'struct tm'!
    addDays(destinationDateTime, dayDifference);
}

/*
Simply converts a MySQL date/time to a 'struct tm'.
*/
void copyMySQLTimeToStructTm(MYSQL_TIME *src, struct tm *dst) {
    dst->tm_year = src->year - 1900;
    dst->tm_mon = src->month - 1;
    dst->tm_mday = src->day;
    dst->tm_hour = src->hour;
    dst->tm_min = src->minute;
    dst->tm_sec = src->second;
    mktime(dst);
}

void copyStructTmToMySQLTime(struct tm *src, MYSQL_TIME *dst) {
    dst->year = src->tm_year + 1900;
    dst->month = src->tm_mon + 1;
    dst->day = src->tm_mday;
    dst->hour = src->tm_hour;
    dst->minute = src->tm_min;
    dst->second = src->tm_sec;
    dst->neg = FALSE;
    dst->second_part = 0;
}
