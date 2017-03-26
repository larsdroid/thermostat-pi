#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "dht22.h"
#include "ini_file.h"

/*
* Based on "dht22.c" that is circulating around the internet. Source and license unknown.
*/

#define MINIMUM_DELAY_BETWEEN_MEASUREMENTS 1000

#define MAXTIMINGS 85

// TODO: the max amount of tries should be in the INI file
static const int maxTries = 100;

static int dht22_dat[5] = {0, 0, 0, 0, 0};

static uint8_t sizecvt(const int read) {
    /* digitalRead() and friends from wiringpi are defined as returning a value
    < 256. However, they are returned as int() types. This is a safety function */

    if (read > 255 || read < 0) {
        printf("Invalid data from wiringPi library.\n");
        exit(EXIT_FAILURE);
    }
    return (uint8_t)read;
}

TEMP_HUM measureTemperatureHumidity() {
    TEMP_HUM result;
    result.temperature = INVALID_MEASURED_TEMPERATURE;
    result.humidity = INVALID_MEASURED_HUMIDITY;

    int currentTries = maxTries;

    do {
        if (currentTries != maxTries) {
            delay(MINIMUM_DELAY_BETWEEN_MEASUREMENTS);
        }

        uint8_t laststate = HIGH;
        uint8_t counter = 0;
        uint8_t j = 0, i;

        dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

        // pull pin down for 18 milliseconds
        pinMode(pinDHT22, OUTPUT);
        digitalWrite(pinDHT22, HIGH);
        delay(10);
        digitalWrite(pinDHT22, LOW);
        delay(18);
        // then pull it up for 40 microseconds
        digitalWrite(pinDHT22, HIGH);
        delayMicroseconds(40);
        // prepare to read the pin
        pinMode(pinDHT22, INPUT);

        // detect change and read data
        for (i = 0; i < MAXTIMINGS; i++) {
            counter = 0;
            while (sizecvt(digitalRead(pinDHT22)) == laststate) {
                counter++;
                delayMicroseconds(1);
                if (counter == 255) {
                    break;
                }
            }
            laststate = sizecvt(digitalRead(pinDHT22));

            if (counter == 255) {
                break;
            }

            // ignore first 3 transitions
            if ((i >= 4) && (i % 2 == 0)) {
                // shove each bit into the storage bytes
                dht22_dat[j / 8] <<= 1;
                if (counter > 16) {
                    dht22_dat[j / 8] |= 1;
                }
                j++;
            }
        }

        // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
        // print it out if data is good
        if ((j >= 40) && (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) ) {
            result.humidity = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];
            result.humidity /= 10;
            result.temperature = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];
            result.temperature /= 10.0;

            if ((dht22_dat[2] & 0x80) != 0) {
                result.temperature *= -1;
            }
            //printf("Humidity = %.2f %% Temperature = %.2f *C \n", result.humidity, result.temperature);
        }
        /*else {
            printf("One failed temperature poll\n");
        }*/
    } while ((result.temperature == INVALID_MEASURED_TEMPERATURE || result.temperature < -20 || result.temperature > 50) && --currentTries);

    // TODO: to be fixed in HARDWARE!
    if (result.temperature != INVALID_MEASURED_TEMPERATURE) {
        result.temperature -= 3.0;
    }

    return result;
}
