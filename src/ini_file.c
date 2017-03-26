#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ini_file.h"

#define TRUE 1
#define FALSE 0

int pinButtonUp = -1;
int pinButtonDown = -1;
int pinButtonMode = -1;
int pinDHT22 = -1;
int pinRelay = -1;
int i2cAddress = -1;
char* mysqlHost = (char *)-1;
char* mysqlUser = (char *)-1;
char* mysqlPasswd = (char *)-1;
char* mysqlDatabase = (char *)-1;
int mysqlPort = -1;
char* mysqlSocket = (char *)-1;

#define INI_FILE "thermostat.ini"
#define PIN_PROPERTY_PREFIX "pin_"
#define BUFFER_SIZE 200

static void readProperty(const char* propertyName, const char* propertyValue, void* ptr) {
    long result;

    int isNumeric = TRUE;
    int isHex = FALSE;
    for (int i = 0; i < strlen(propertyValue); i++) {
        if (i == 1 && propertyValue[0] == '0' && propertyValue[1] == 'x') {
            isHex = TRUE;
            isNumeric = FALSE;
            break;
        }
        else if (propertyValue[i] < '0' || propertyValue[i] > '9') {
            isNumeric = FALSE;
            break;
        }
    }

    result = strtol(propertyValue, NULL, 10);

    if (!isNumeric && !isHex) {
        // It's not a valid number so interpret it as a string property

        // This memory doesn't need to be freed
        *(char **)ptr = calloc(strlen(propertyValue) + 1, sizeof(char));
        strcpy(*(char **)ptr, propertyValue);
    }
    else if (isHex) {
        // HEX
        int factor = 1;
        int decimalValue = 0;
        if (strlen(propertyValue) < 3 || strlen(propertyValue) > 6) {
            fprintf(stderr, "Length of \"%s\" should be between 1 and 4 hexadecimal digits (inclusive). Currently: %d.\n", propertyName, (int)strlen(propertyValue) - 2);
            exit(1);
        }
        for (int i = strlen(propertyValue) - 1; i >= 2; i--) {
            if (!((propertyValue[i] >= 48 && propertyValue[i] <= 57) || (propertyValue[i] >= 65 && propertyValue[i] <= 70))) {
                fprintf(stderr, "\"%s\" should only contain hexadecimal digits (upper case).\n", propertyName);
                exit(1);
            }
            if (propertyValue[i] >= 48 && propertyValue[i] <= 57) {
                // 0-9
                decimalValue += factor * (propertyValue[i] - 48);
            }
            else {
                // A-F
                decimalValue += factor * (propertyValue[i] - 55);
            }

            factor *= 16;
        }
        *(int *)ptr = decimalValue;
    }
    else {
        // The property is a valid number
        if (strncmp(propertyName, PIN_PROPERTY_PREFIX, strlen(PIN_PROPERTY_PREFIX)) == 0 && (result < 0 || result > 29)) {
            fprintf(stderr, "Value for \"%s\" should be in range [0, 29]. Currently: %ld.\n", propertyName, result);
            exit(1);
        }

        if (result < 0) {
            fprintf(stderr, "Value for \"%s\" should be > 0. Currently: %ld.\n", propertyName, result);
            exit(1);
        }

        *(int *)ptr = (int) result;
    }
}

void readIniFile() {
    FILE* filePtr = fopen(INI_FILE, "r");

    if (filePtr == NULL) {
        fprintf(stderr, "Can't open ini file \"%s\" for reading.\n", INI_FILE);
        exit(1);
    }

    // The possible settings and the variables that they initialise:
    const char* properties[] = {
        "pin_button_up",
        "pin_button_down",
        "pin_button_mode",
        "pin_dht22",
        "pin_relay",
        "i2c_address",
        "mysql_host",
        "mysql_user",
        "mysql_passwd",
        "mysql_database",
        "mysql_port",
        "mysql_socket"
    };
    void* values[] = {
        &pinButtonUp,
        &pinButtonDown,
        &pinButtonMode,
        &pinDHT22,
        &pinRelay,
        &i2cAddress,
        &mysqlHost,
        &mysqlUser,
        &mysqlPasswd,
        &mysqlDatabase,
        &mysqlPort,
        &mysqlSocket
    };
    const int propertiesCount = sizeof(properties) / sizeof(const char *);

    if (propertiesCount != (sizeof(values) / sizeof(void *))) {
        fprintf(stderr, "Error in ini_file.c: properties array doesn't have the same length as values array.\n");
        exit(1);
    }

    char lineBuffer[BUFFER_SIZE];
    int i, j;
    int lineCount = 0;
    char* value;
    while (fgets(lineBuffer, BUFFER_SIZE, filePtr) != NULL) {
        lineCount++;

        // Skip comment lines (#) and empty lines.
        if (lineBuffer[0] == '#' || lineBuffer[0] == 10 || lineBuffer[0] == 13) {
            continue;
        }

        i = 0;
        while (lineBuffer[i] != '=' && i < BUFFER_SIZE - 2) {
            i++;
        }
        if (i == 0 || lineBuffer[i] != '=') {
            fprintf(stderr, "Error searching for \"=\" on line %d.\n", lineCount);
            exit(1);
        }
        lineBuffer[i] = '\0';
        i++;
        value = &lineBuffer[i];
        while (lineBuffer[i] != 10 && lineBuffer[i] != 13 && i < BUFFER_SIZE - 1) {
            i++;
        }
        lineBuffer[i] = '\0';

        for (j = 0; j < propertiesCount; j++) {
            if (strcmp(lineBuffer, properties[j]) == 0) {
                readProperty(properties[j], value, values[j]);
            }
        }
    }

    // Check if all pin variables have been successfully initialised.
    for (j = 0; j < propertiesCount; j++) {
        if (*((int *)values[j]) == -1) {
            fprintf(stderr, "No value specified for \"%s\" in %s.\n", properties[j], INI_FILE);
            exit(1);
        }
    }
}
