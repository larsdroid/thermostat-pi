#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <wiringPi.h>

#include "ini_file.h"
#include "util.h"
#include "display.h"
#include "dht22.h"
#include "db.h"

// This is an internal value that usually shouldn't altered. It denotes the amount of milliseconds before a push button's press/release has become stable.
#define BUTTON_STABLE_DELAY 30.0
#define BLINK_MILLIS 350
#define BLINK_DURATION_MILLIS 5000
#define BACKLIGHT_DURATION_MILLIS 10000 // It is assumed that the backlight is enabled for LONGER than the blink duration! So BACKLIGHT_DURATION_MILLIS should be greater than BLINK_DURATION_MILLIS.
#define TEMP_HUM_POLL_FREQUENCY 60000

static TEMP_HUM currentTempHum;
static volatile int isHeatingOn;
static volatile int isDisplayBlinking = FALSE;
static volatile int isDisplayBlinkOn = FALSE;
static volatile float requestedTemperature;
static volatile float temporaryRequestedTemperature;
static volatile double timeMillisButtonUpLastPressed = 0.0;
static volatile double timeMillisButtonDownLastPressed = 0.0;
static volatile double timeMillisButtonModeLastPressed = 0.0;
static volatile int isButtonUpPressed = FALSE;
static volatile int isButtonDownPressed = FALSE;
static volatile int isButtonModePressed = FALSE;

static pthread_mutex_t mutexBoiler;

static int isBacklightDurationExpired() {
    const double currentTimeMillis = getTimeMillis();
    if ((currentTimeMillis - timeMillisButtonUpLastPressed > BACKLIGHT_DURATION_MILLIS || timeMillisButtonUpLastPressed - currentTimeMillis > BACKLIGHT_DURATION_MILLIS)
            && (currentTimeMillis - timeMillisButtonDownLastPressed > BACKLIGHT_DURATION_MILLIS || timeMillisButtonDownLastPressed - currentTimeMillis > BACKLIGHT_DURATION_MILLIS)
            && (currentTimeMillis - timeMillisButtonModeLastPressed > BACKLIGHT_DURATION_MILLIS || timeMillisButtonModeLastPressed - currentTimeMillis > BACKLIGHT_DURATION_MILLIS)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

static int isBlinkDurationExpired() {
    const double currentTimeMillis = getTimeMillis();
    if ((currentTimeMillis - timeMillisButtonUpLastPressed > BLINK_DURATION_MILLIS || timeMillisButtonUpLastPressed - currentTimeMillis > BLINK_DURATION_MILLIS)
            && (currentTimeMillis - timeMillisButtonDownLastPressed > BLINK_DURATION_MILLIS || timeMillisButtonDownLastPressed - currentTimeMillis > BLINK_DURATION_MILLIS)
            && (currentTimeMillis - timeMillisButtonModeLastPressed > BLINK_DURATION_MILLIS || timeMillisButtonModeLastPressed - currentTimeMillis > BLINK_DURATION_MILLIS)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

/*
* Parameter 'changeRequestLaunched' denotes whether or not a USER just modified the requested temperature.
* If this is the case then we don't require the requested temperature to be half a degree from the actual
* temperature.
*/
static void checkStartStopBoiler(int changeRequestLaunched) {
    // Only one thread shall run this code at a time:
    pthread_mutex_lock(&mutexBoiler);

    if (isHeatingOn) {
        if ((requestedTemperature <= (currentTempHum.temperature - 0.5)) || (changeRequestLaunched && (requestedTemperature <= currentTempHum.temperature))) {
            // Stop the heating.
            digitalWrite(pinRelay, LOW);
            isHeatingOn = FALSE;
            displayFlame(FALSE);
            insertBoilerActivated(FALSE);
        }
        // else --> do nothing
    }
    else {
        if (requestedTemperature >= (currentTempHum.temperature + 0.5) || (changeRequestLaunched && (requestedTemperature > currentTempHum.temperature))) {
            // Start the heating.
            digitalWrite(pinRelay, HIGH);
            isHeatingOn = TRUE;
            displayFlame(TRUE);
            insertBoilerActivated(TRUE);
        }
        // else --> do nothing
    }

    pthread_mutex_unlock(&mutexBoiler);
}

static void* blinkDisplayThreadWork(void *p) {
    for (;;) {
        if (isBacklightEnabled()) {
            if (isBacklightDurationExpired()) {
                // No button has been pressed for the last 10 seconds --> disable backlight.
                setBacklightEnabled(FALSE);
            }

            if (isDisplayBlinking) {
                int showRequestedTemperatureDigits;
                if (isBlinkDurationExpired()) {
                    // Store the requested temperature.
                    requestedTemperature = temporaryRequestedTemperature;

                    // Log the new request in the database.
                    insertTemperatureRequestButtons(requestedTemperature);

                    // Now show the requested temperature only in case it's different from the actual temperature.
                    showRequestedTemperatureDigits = (requestedTemperature != currentTempHum.temperature);

                    // Update the actual border.
                    checkStartStopBoiler(TRUE);

                    // No button has been pressed for the last 5 seconds --> stop blinking.
                    isDisplayBlinking = FALSE;
                }
                else {
                    showRequestedTemperatureDigits = !isDisplayBlinkOn;
                    isDisplayBlinkOn = !isDisplayBlinkOn;
                }

                if (showRequestedTemperatureDigits) {
                    displayRequestedTemperature(temporaryRequestedTemperature);
                }
                else {
                    displayRequestedTemperature(-1);
                }
            }
        }
        delay(BLINK_MILLIS);
    }

    pthread_exit(NULL);
    return NULL;
}

static void* measureTemperatureUpdateTimeThreadWork(void *p) {
    for (;;) {
        // TODO: currently "isDisplayBlinking" is functioning as some kind of mutex lock. Use a proper mutex here.
        if (!isDisplayBlinking) {
            requestedTemperature = checkAndApplyScheduleActivation();
        }

        currentTempHum = measureTemperatureHumidity();
        if (currentTempHum.temperature != INVALID_MEASURED_TEMPERATURE) {
            // Update the temperature digits:
            displayCurrentTempHum(&currentTempHum);
            insertTemperatureHumidity(&currentTempHum);

            if ((requestedTemperature != currentTempHum.temperature) && !isDisplayBlinking) {
                displayRequestedTemperature(requestedTemperature);
            }

            checkStartStopBoiler(FALSE);
        }

        // Update the date and time digits (if necessary):
        checkUpdateDateTimeDisplay();

        delay(TEMP_HUM_POLL_FREQUENCY);
    }

    pthread_exit(NULL);
    return NULL;
}

static void buttonUpPressed() {
    if (isDisplayBlinking) {
        // The display is blinking and the requested temperature is showing so it's time to update the requested temperature.
        temporaryRequestedTemperature += 0.5;
    }
    else {
        // At this point temporaryRequestedTemperature should already be equal to requestedTemperature. (maybe add an assert later)
        temporaryRequestedTemperature = requestedTemperature;

        isDisplayBlinking = TRUE;
    }
}

static void buttonDownPressed() {
    if (isDisplayBlinking) {
        // The display is blinking and the requested temperature is showing so it's time to update the requested temperature.
        temporaryRequestedTemperature -= 0.5;
    }
    else {
        // At this point temporaryRequestedTemperature should already be equal to requestedTemperature. (maybe add an assert later)
        temporaryRequestedTemperature = requestedTemperature;

        isDisplayBlinking = TRUE;
    }
}

static void buttonModePressed() {
    // Nothing should be done here, this button is currently only used to fire up the backlight.
}

// Not used for now.
static void buttonUpReleased() {
}

// Not used for now.
static void buttonDownReleased() {
}

// Not used for now.
static void buttonModeReleased() {
}

static void handleButtonEvent(int *buttonPin, volatile double *timeMillisButtonLastPressed, volatile int *isButtonPressed, void (*handlePressed)(), void (*handleReleased)()) {
    const double currentTimeMillis = getTimeMillis();
    int pinValue = digitalRead(*buttonPin);
    if (currentTimeMillis - *timeMillisButtonLastPressed > BUTTON_STABLE_DELAY || *timeMillisButtonLastPressed - currentTimeMillis > BUTTON_STABLE_DELAY) {
        if (!pinValue && !*isButtonPressed) {
            // No matter which button is pressed, the backlight should be enabled (for a while).
            setBacklightEnabled(TRUE);

            handlePressed();
            *isButtonPressed = TRUE;
            *timeMillisButtonLastPressed = currentTimeMillis;
        }
        else if (pinValue && *isButtonPressed) {
            // Do don't do anything with the backlight for now.

            // The button release functions aren't used for now (see buttonUpReleased, buttonDownReleased and buttonModeReleased above).
            //handleReleased();

            *isButtonPressed = FALSE;
            *timeMillisButtonLastPressed = currentTimeMillis;
        }
    }
}

static void buttonUpHandleEvent() {
    handleButtonEvent(&pinButtonUp, &timeMillisButtonUpLastPressed, &isButtonUpPressed, &buttonUpPressed, &buttonUpReleased);
}

static void buttonDownHandleEvent() {
    handleButtonEvent(&pinButtonDown, &timeMillisButtonDownLastPressed, &isButtonDownPressed, &buttonDownPressed, &buttonDownReleased);
}

static void buttonModeHandleEvent() {
    handleButtonEvent(&pinButtonMode, &timeMillisButtonModeLastPressed, &isButtonModePressed, &buttonModePressed, &buttonModeReleased);
}

static void initWiringPi() {
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "Couldn't set up WiringPi.\n");
        exit(1);
    }

    pinMode(pinButtonUp, INPUT);
    pullUpDnControl(pinButtonUp, PUD_UP);
    wiringPiISR(pinButtonUp, INT_EDGE_BOTH, &buttonUpHandleEvent);

    pinMode(pinButtonDown, INPUT);
    pullUpDnControl(pinButtonDown, PUD_UP);
    wiringPiISR(pinButtonDown, INT_EDGE_BOTH, &buttonDownHandleEvent);

    pinMode(pinButtonMode, INPUT);
    pullUpDnControl(pinButtonMode, PUD_UP);
    wiringPiISR(pinButtonMode, INT_EDGE_BOTH, &buttonModeHandleEvent);

    pinMode(pinRelay, OUTPUT);
}

static void initDisplay() {
    requestedTemperature = 20.0; // Will be replaced by a value from the DB
    temporaryRequestedTemperature = requestedTemperature;

    initalizeDisplay();
}

int main(int argc, char const *argv[]) {
    // TODO: respond to SIG INT HUP and TERM
    //       https://www.gnu.org/software/libc/manual/html_node/Sigaction-Function-Example.html

    readIniFile();
    initalizeDatabase();
    initWiringPi();
    initDisplay();

    isHeatingOn = FALSE;

    // From this point on all boiler code should be mutexed.
    pthread_mutex_init(&mutexBoiler, NULL);

    pthread_t threadBlinkDisplay;
    if (pthread_create(&threadBlinkDisplay, NULL, blinkDisplayThreadWork, NULL)) {
        printf("Unable to create blink display thread.\n");
        exit(1);
    }

    pthread_t threadTemperature;
    if (pthread_create(&threadTemperature, NULL, measureTemperatureUpdateTimeThreadWork, NULL)) {
        printf("Unable to create temperature measuring thread.\n");
        exit(1);
    }

    pthread_exit(NULL);
    return 0;
}
