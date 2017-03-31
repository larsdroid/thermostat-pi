#ifndef DISPLAY_H
#define DISPLAY_H

#include "dht22.h"

void setBacklightEnabled(int);
int isBacklightEnabled();
void initalizeDisplay();

// Is supposed to be called once per minute:
void checkUpdateDateTimeDisplay();

void displayCurrentTempHum(TEMP_HUM *);

// Parameter should be -1 to display no requested temperature:
void displayRequestedTemperature(float);

void displayFlame(int);

#endif /* DISPLAY_H */
