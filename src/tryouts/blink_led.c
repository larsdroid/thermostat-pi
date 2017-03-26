#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wiringPi.h>

#define DELAY_MILLIS 1000

int main(int argc, const char* argv[]) {
    int pin;
    if (argc == 2) {
        char *eptr;
        long result;
        char *zero = "0";
        
        result = strtol(argv[1], &eptr, 10);
        
        if (result == 0 && strcmp(argv[1], zero) != 0) {
            printf("Invalid argument: %s\n", argv[1]);
            return 1;
        }
        
        pin = (int)result;
    }
    else {
        printf("Missing argument.\n");
        return 2;
    }
    
    printf("Blinking LED.\n");

    wiringPiSetup();
    pinMode(pin, OUTPUT);
    
    while (1) {
        digitalWrite(pin, LOW);
        delay(DELAY_MILLIS);
        digitalWrite(pin, HIGH);
        delay(DELAY_MILLIS);
    }

    return 0;
}

