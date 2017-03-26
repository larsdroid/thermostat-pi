#include <stdio.h>

#include "ini_file.h"

int main(int argc, char const *argv[]) {
    readIniFile();

    printf("Values retrieved from ini file:\n");
    printf("  PIN BUTTON UP:   %d\n", pinButtonUp);
    printf("  PIN BUTTON DOWN: %d\n", pinButtonDown);
    printf("  PIN BUTTON MODE: %d\n", pinButtonMode);
    printf("  PIN DHT22:       %d\n", pinDHT22);
    printf("  PIN RELAY:       %d\n", pinRelay);
    printf("  I2C ADDRESS:     %d\n", i2cAddress);
    printf("  MYSQL HOST:      %s\n", mysqlHost);
    printf("  MYSQL USER:      %s\n", mysqlUser);
    printf("  MYSQL PASSWORD:  %s\n", mysqlPasswd);
    printf("  MYSQL DATABASE:  %s\n", mysqlDatabase);
    printf("  MYSQL PORT:      %d\n", mysqlPort);
    printf("  MYSQL SOCKET:    %s\n", mysqlSocket);

    return 0;
}
