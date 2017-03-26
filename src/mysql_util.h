#ifndef MYSQL_UTIL_H
#define MYSQL_UTIL_H

#include <time.h>

typedef unsigned int uint;
typedef unsigned long ulong;
#include <my_global.h>
#include <mysql.h>

MYSQL *con;

void closeMySQLDatabase();
void initalizeMySQLDatabase();

void reportMySQLError();
void reportMySQLStatementError(MYSQL_STMT *);
void reportGenericError(const char *errorMessage);

int initializeAndExecuteStatement(char * sql, size_t sqlLength, MYSQL_BIND *param, MYSQL_BIND *result);
int containsNullColumn(MYSQL_BIND *result, int columnCount);

void addBindParameterLongLong(MYSQL_BIND *param, long long int *value);
void addBindParameterTimestamp(MYSQL_BIND *param, MYSQL_TIME *value);
void addBindParameterFloat(MYSQL_BIND *param, float *value);
void addBindParameterString(MYSQL_BIND *param, const char *value, unsigned long *length);
void addBindParameterTiny(MYSQL_BIND *param, signed char *value);

void addBindResultLongLong(MYSQL_BIND *result, my_bool *isNull, long long int *value);
void addBindResultFloat(MYSQL_BIND *result, my_bool *isNull, float *value);
void addBindResultTimestamp(MYSQL_BIND *result, my_bool *isNull, MYSQL_TIME *value);
void addBindResultTiny(MYSQL_BIND *result, my_bool *isNull, signed char *value);

void convertMySQLTimeToDateTime(MYSQL_TIME *sourceTime, signed char sourceDayOfWeek, struct tm *destinationDateTime, struct tm *localDateTime);
void copyMySQLTimeToStructTm(MYSQL_TIME *src, struct tm *dst);
void copyStructTmToMySQLTime(struct tm *src, MYSQL_TIME *dst);

#endif /* MYSQL_UTIL_H */
