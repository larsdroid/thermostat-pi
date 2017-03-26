#include <wiringPi.h>
#include <pcf8574.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

// Part of WiringPi dev
#include "lcd.h"

#include "display.h"
#include "ini_file.h"

#define DEGREES_SYMBOL 0     // °
#define FLAME_SYMBOL   1

#define AF_BASE 100

#define AF_RS   (AF_BASE + 0)
#define AF_RW   (AF_BASE + 1)
#define AF_E    (AF_BASE + 2)

#define AF_BL   (AF_BASE + 3)

#define AF_D1   (AF_BASE + 4)
#define AF_D2   (AF_BASE + 5)
#define AF_D3   (AF_BASE + 6)
#define AF_D4   (AF_BASE + 7)

unsigned char degrees[8] = {
    0b00100,
    0b01010,
    0b00100,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};

unsigned char flame[8] = {
    0b00100,
    0b00110,
    0b01010,
    0b01001,
    0b10101,
    0b10101,
    0b10001,
    0b01110
};

static int screen;
static volatile int isBacklightOn;
static volatile int displayedHour; // Only hour (the minutes are updated EVERY time (every minute...))
static volatile int displayedDayOfMonth;
static volatile float displayedCurrentTemperature;
static volatile float displayedRequestedTemperature; // -1 in case no requested temperature is displayed

static pthread_mutex_t mutexDisplay;

static const char* weekdays[] = {"zondag", "maandag", "dinsdag", "woensdag", "donderdag", "vrijdag", "zaterdag"};
static const char* months[] = {"januari", "februari", "maart", "april", "mei", "juni", "juli", "augustus", "september", "oktober", "november", "december"};
static const char* monthsAbbr[] = {"jan.", "feb.", "maa.", "apr.", "mei", "jun.", "jul.", "aug.", "sep.", "okt.", "nov.", "dec."};

void setBacklightEnabled(int enable) {
    if ((enable && !isBacklightOn) || (!enable && isBacklightOn)) {
        pthread_mutex_lock(&mutexDisplay);
        if (enable) {
            digitalWrite(AF_BL, 1);
        }
        else {
            digitalWrite(AF_BL, 0);
        }
        isBacklightOn = enable;
        pthread_mutex_unlock(&mutexDisplay);
    }
}

int isBacklightEnabled() {
    return isBacklightOn;
}

/*
* ______________________
* |donderdag 15 april  |
* |15:46    45%   19.7°|
* |           (*) 20.0^|
* |                    |
* ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
*/
void checkUpdateDateTimeDisplay() {
    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    int currentHour = localTime->tm_hour;
    int currentMinutes = localTime->tm_min;
    int currentDayOfMonth = localTime->tm_mday;

    if (currentHour != displayedHour) {
        char hourString[3];
        sprintf(hourString, "%2d", currentHour);

        pthread_mutex_lock(&mutexDisplay);
        lcdPosition(screen, 0, 1);
        lcdPuts(screen, hourString);
        displayedHour = currentHour;
        pthread_mutex_unlock(&mutexDisplay);

        if (currentDayOfMonth != displayedDayOfMonth) {
            char dateString[21];
            int dayOfWeek = localTime->tm_wday; // 0-6
            int month = localTime->tm_mon; // 0-11

            int index = strlen(weekdays[dayOfWeek]);
            strncpy(dateString, weekdays[dayOfWeek], index);
            dateString[index++] = ' ';
            sprintf(&dateString[index++], "%d", currentDayOfMonth);
            if (currentDayOfMonth > 9) {
                index++;
            }
            dateString[index++] = ' ';
            if (index + strlen(months[month]) <= 20) {
                strncpy(&dateString[index], months[month], strlen(months[month]));
                index += strlen(months[month]);
            }
            else {
                strncpy(&dateString[index], monthsAbbr[month], strlen(monthsAbbr[month]));
                index += strlen(monthsAbbr[month]);
            }
            while (index < 20) {
                dateString[index++] = ' ';
            }
            dateString[index++] = '\0';

            pthread_mutex_lock(&mutexDisplay);
            lcdPosition(screen, 0, 0);
            lcdPuts(screen, dateString);
            displayedDayOfMonth = currentDayOfMonth;
            pthread_mutex_unlock(&mutexDisplay);
        }
    }

    char minuteString[3];
    sprintf(minuteString, "%02d", currentMinutes);

    pthread_mutex_lock(&mutexDisplay);
    lcdPosition(screen, 3, 1);
    lcdPuts(screen, minuteString);
    pthread_mutex_unlock(&mutexDisplay);
}

void displayCurrentTempHum(TEMP_HUM *th) {
    if (displayedCurrentTemperature != th->temperature) {
        char tempString[5];
        sprintf(tempString, "%2.1f", th->temperature);
        char humString[3];
        sprintf(humString, "%2.0f", th->humidity);

        pthread_mutex_lock(&mutexDisplay);
        lcdPosition(screen, 15, 1);
        lcdPuts(screen, tempString);
        lcdPosition(screen, 9, 1);
        lcdPuts(screen, humString);
        displayedCurrentTemperature = th->temperature;
        pthread_mutex_unlock(&mutexDisplay);
    }
}

void displayFlame(int on) {
    pthread_mutex_lock(&mutexDisplay);
    lcdPosition(screen, 12, 2);
    if (on) {
        lcdPutchar(screen, FLAME_SYMBOL);
    }
    else {
        lcdPutchar(screen, ' ');
    }
    pthread_mutex_unlock(&mutexDisplay);
}

void displayRequestedTemperature(float requestedTemperature) {
    if (requestedTemperature != displayedRequestedTemperature) {
        char reqTempString[5];
        if (requestedTemperature == -1) {
            reqTempString[0] = reqTempString[1] = reqTempString[2] = reqTempString[3] = ' ';
            reqTempString[4] = '\0';
        }
        else {
            sprintf(reqTempString, "%2.1f", requestedTemperature);
        }

        pthread_mutex_lock(&mutexDisplay);
        lcdPosition(screen, 15, 2);
        lcdPuts(screen, reqTempString);
        lcdPosition(screen, 19, 2);
        if (requestedTemperature == -1) {
            // Don't show the degrees symbol.
            lcdPutchar(screen, ' ');
        }
        else {
            lcdPutchar(screen, DEGREES_SYMBOL);
        }
        displayedRequestedTemperature = requestedTemperature;
        pthread_mutex_unlock(&mutexDisplay);
    }
}

void initalizeDisplay() {
    pcf8574Setup(AF_BASE, i2cAddress);

    screen = lcdInit(4, 20, 4,
        AF_RS, AF_E,
        AF_D1, AF_D2, AF_D3, AF_D4, 0, 0, 0, 0);

    // Backlight pin is an output pin
    pinMode(AF_BL, OUTPUT);

    //Set LCD into write mode.
    pinMode(AF_RW, OUTPUT);
    digitalWrite(AF_RW, 0);

    setBacklightEnabled(FALSE);

    lcdCharDef(screen, DEGREES_SYMBOL, degrees);
    lcdCharDef(screen, FLAME_SYMBOL, flame);

    lcdClear(screen);

    displayedHour = -1;
    displayedDayOfMonth = -1;
    displayedCurrentTemperature = -1;
    displayedRequestedTemperature = -1;

    lcdPosition(screen, 2, 1);
    lcdPutchar(screen, ':');
    lcdPosition(screen, 11, 1);
    lcdPutchar(screen, '%');
    lcdPosition(screen, 19, 1);
    lcdPutchar(screen, DEGREES_SYMBOL);
    lcdPosition(screen, 11, 2);
    lcdPutchar(screen, '(');
    lcdPosition(screen, 13, 2);
    lcdPutchar(screen, ')');

    // From this point on all LCD code should be mutexed.
    pthread_mutex_init(&mutexDisplay, NULL);
}
