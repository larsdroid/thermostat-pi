#ifndef DHT22_H
#define DHT22_H

typedef struct {
    float temperature;
    float humidity;
} TEMP_HUM;

#define INVALID_MEASURED_TEMPERATURE -100
#define INVALID_MEASURED_HUMIDITY -1

/*
* Returns a temperature equal to INVALID_MEASURED_TEMPERATURE in case something went wrong.
*/
TEMP_HUM measureTemperatureHumidity();

#endif /* DHT22_H */
