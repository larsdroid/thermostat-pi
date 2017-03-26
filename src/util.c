#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"

#define TRUE 1
#define FALSE 0

static const char* weekdays[] = {"zondag", "maandag", "dinsdag", "woensdag", "donderdag", "vrijdag", "zaterdag"};

int get_pin_nr(int argc, const char* argv[]) {
    if (argc == 2) {
        char *eptr;
        long result;
        char zero[] = "0";

        result = strtol(argv[1], &eptr, 10);

        if (result == 0 && strcmp(argv[1], zero) != 0) {
            printf("Invalid argument: %s\n", argv[1]);
            return 1;
        }

        return (int)result;
    }
    else {
        printf("Missing argument.\n");
        return -1;
    }
}

double getTimeMillis() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}

struct tm getLocalTime() {
    time_t rawtime;
    time(&rawtime);
    return *localtime(&rawtime);
}

void addDays(struct tm *dateTime, int daysToAdd) {
    dateTime->tm_mday += daysToAdd;
    mktime(dateTime);
}

void printDateTime(struct tm *dateTime) {
    printf("%04d-%02d-%02d %02d:%02d:%02d (%s)",
            dateTime->tm_year + 1900,
            dateTime->tm_mon + 1,
            dateTime->tm_mday,
            dateTime->tm_hour,
            dateTime->tm_min,
            dateTime->tm_sec,
            weekdays[dateTime->tm_wday]
            );
}

/*
Returns TRUE if a is before b.
*/
int isBefore(struct tm *a, struct tm *b) {
    if (a->tm_year < b->tm_year) {
        return TRUE;
    }
    else if (a->tm_year > b->tm_year) {
        return FALSE;
    }
    else {
        if (a->tm_mon < b->tm_mon) {
            return TRUE;
        }
        else if (a->tm_mon > b->tm_mon) {
            return FALSE;
        }
        else {
            if (a->tm_mday < b->tm_mday) {
                return TRUE;
            }
            else if (a->tm_mday > b->tm_mday) {
                return FALSE;
            }
            else {
                if (a->tm_hour < b->tm_hour) {
                    return TRUE;
                }
                else if (a->tm_hour > b->tm_hour) {
                    return FALSE;
                }
                else {
                    if (a->tm_min < b->tm_min) {
                        return TRUE;
                    }
                    else if (a->tm_min > b->tm_min) {
                        return FALSE;
                    }
                    else {
                        if (a->tm_sec < b->tm_sec) {
                            return TRUE;
                        }
                        else {
                            return FALSE;
                        }
                    }
                }
            }
        }
    }
}
