#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>

#include "dht22.h"
#include "ini_file.h"

int main(int argc, char const *argv[]) {
    readIniFile();

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "Couldn't set up WiringPi.\n");
        exit(1);
    }

    printf("Polling for temperature/humidity on GPIO %d (WiringPi numbering).\n", pinDHT22);

    TEMP_HUM th = measureTemperatureHumidity();
    printf("Temperature = %.1f  ---  Humidity = %.1f\n", th.temperature, th.humidity);

    return 0;
}
