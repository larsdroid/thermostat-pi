#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <time.h>

int get_pin_nr(int argc, const char* argv[]);
double getTimeMillis();
struct tm getLocalTime();
void addDays(struct tm *dateTime, int daysToAdd);
void printDateTime(struct tm *dateTime);
int isBefore(struct tm *a, struct tm *b);

#endif /* ARGUMENTS_H */
