#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>

#include "util.h"
#include "ini_file.h"

#define BUTTON_STABLE_DELAY 30.0

// Time is expressed in milliseconds
static double buttonLastPressed = 0.0;
static int pin = -1;
static int buttonIsPressed = FALSE;

/*
* TODO: When another button is pressed (or when some other event is triggered) some extra check are to be added.
*       In case the first button is STILL pressed when another button is pressed it may be that the first
*       button is in an invalid state (buttonIsPressed has an incorrect value). In this case the buttonIsPressed
*       variable's value can simply be corrected. If simultaneous button presses are to be supported then an
*       extra time-comparison check should be added on top of that.
*/
static void button_is_pressed(void) {
    const double currentTimeMillis = getTimeMillis();
    int pinValue = digitalRead(pin);
    if (currentTimeMillis - buttonLastPressed > BUTTON_STABLE_DELAY || buttonLastPressed - currentTimeMillis > BUTTON_STABLE_DELAY) {
        if (!pinValue && !buttonIsPressed) {
            printf("I am PRESSED, current PIN READ = %d\n", pinValue);
            fflush(stdout);
            buttonIsPressed = TRUE;
            buttonLastPressed = currentTimeMillis;
        }
        else if (pinValue && buttonIsPressed) {
            printf("     RELEASED, current PIN READ = %d\n", pinValue);
            fflush(stdout);
            buttonIsPressed = FALSE;
            buttonLastPressed = currentTimeMillis;
        }
    }
}

int main(int argc, char const *argv[]) {
    pin = get_pin_nr(argc, argv);

    if (pin >= 0) {
        printf("Using pin number from command line argument.\n");
    }
    else {
        printf("Using pin number from ini file.\n");
        readIniFile();
        pin = pinButtonUp;
    }

    printf("Polling for button press on GPIO %d (WiringPi numbering).\n", pin);

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "Couldn't set up WiringPi.\n");
        exit(1);
    }

    pinMode(pin, INPUT);
    pullUpDnControl(pin, PUD_UP);
    wiringPiISR(pin, INT_EDGE_BOTH, &button_is_pressed);

    /*
    * PUD_UP --- INT_EDGE_BOTH --- input pin to button leg, button leg to GROUND
    * ---> When button is pressed --> the event is triggered
    */

    /*
    * Note: On BananaPi PIN 8 didn't work, PIN 0 did work (On PiZero both worked fine)
    */

    printf("In the main thread I won't be doing anything.\n");
    for (;;) {
        delay(5000);
        printf("Main thread is still alive. Going to sleep for 5 seconds...\n");
        fflush(stdout);
    }

    return 0;
}
