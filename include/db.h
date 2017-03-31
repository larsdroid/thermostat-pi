#ifndef DB_H
#define DB_H

#include <time.h>

#include "dht22.h"

#define INVALID_ID -1
#define INVALID_FETCHED_TEMPERATURE -1.0f

typedef struct {
    float requestedTemperature;
    struct tm from;
} SCHEDULE;

#define REQUEST_SOURCE_BUTTONS  0
#define REQUEST_SOURCE_SCHEDULE 1
#define REQUEST_SOURCE_ANDROID  2
#define REQUEST_SOURCE_WEB      3

void insertTemperatureHumidity(TEMP_HUM *th);
void insertBoilerActivated(int on);
void insertTemperatureRequest(SCHEDULE *schedule, int requestSource);
void insertTemperatureRequestButtons(float);
float checkAndApplyScheduleActivation();
SCHEDULE fetchTemperatureRequest(struct tm *localTime);
SCHEDULE fetchSchedule(struct tm *localTime);
long long int fetchFixedScheduleParent(struct tm *localTime);
SCHEDULE fetchFixedSchedule(long long int scheduleId, struct tm *localTime);
SCHEDULE fetchOneTimeSchedule(struct tm *localTime);

// Generic functions:
void closeDatabase();
void initalizeDatabase();

// Test functions:
void deleteTempHumFrom1900();
void insertTempHumFrom1900();
void printCountTempHumFrom1900();
void printTempHumFrom1900();

#endif /* DB_H */
