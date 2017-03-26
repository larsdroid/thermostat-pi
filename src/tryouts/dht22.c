#include <wiringPi.h>

typedef unsigned int uint;

#include <my_global.h>
#include <mysql.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXTIMINGS 85

static int DHTPIN = 0;
static int dht22_dat[5] = {0, 0, 0, 0, 0};

static uint8_t sizecvt(const int read) {
    /* digitalRead() and friends from wiringpi are defined as returning a value
    < 256. However, they are returned as int() types. This is a safety function */

    if (read > 255 || read < 0) {
        printf("Invalid data from wiringPi library\n");
        exit(EXIT_FAILURE);
    }
    return (uint8_t)read;
}

static int writeToDB(float temp, float hum) {
    MYSQL *con = mysql_init(NULL);

    if (con == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con));
        return 0;
    }

    if (mysql_real_connect(con, "localhost", "anonymous", "anonymous", "banana", 0, "/var/run/mysqld/mysqld.sock", 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        return 0;
    }

    char sql_statement[75];
    sprintf(sql_statement, "INSERT INTO temp_hum (th_temperature, th_humidity) VALUES (%.2f, %.2f)", temp, hum);
   
    if (mysql_query(con, sql_statement)) {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        return 0;
    }

    mysql_close(con);
    return 1;
}

static int read_dht22_dat() {
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

    // pull pin down for 18 milliseconds
    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, HIGH);
    delay(10);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    // then pull it up for 40 microseconds
    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(40); 
    // prepare to read the pin
    pinMode(DHTPIN, INPUT);

    // detect change and read data
    for ( i=0; i< MAXTIMINGS; i++) {
        counter = 0;
        while (sizecvt(digitalRead(DHTPIN)) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;
            }
        }
        laststate = sizecvt(digitalRead(DHTPIN));

        if (counter == 255) {
            break;
        }

        // ignore first 3 transitions
        if ((i >= 4) && (i%2 == 0)) {
            // shove each bit into the storage bytes
            dht22_dat[j/8] <<= 1;
            if (counter > 16) {
                dht22_dat[j/8] |= 1;
            }
            j++;
        }
    }

    // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
    // print it out if data is good
    if ((j >= 40) && 
                (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) ) {
        float t, h;
        h = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];
        h /= 10;
        t = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];
        t /= 10.0;
        
        if ((dht22_dat[2] & 0x80) != 0) {
            t *= -1;
        }

        // INSERT INTO temp_hum (th_temperature, th_humidity) VALUES (12.50, 12.34);
        printf("Humidity = %.2f %% Temperature = %.2f *C \n", h, t );
        writeToDB(t, h);
        return 1;
    }
    else {
        printf("Data not good, skip\n");
        return 0;
    }
}

int main (int argc, char *argv[]) {
    int tries = 100;

    if (argc < 2) {
        printf("usage: %s <pin> (<tries>)\n", argv[0]);
        printf("description: pin is the wiringPi pin number\n");
        printf("    using 0 (GPIO 17)\n");
        printf("Optional: tries is the number of times to try to obtain a read (default 100)\n");
    }
    else {
        DHTPIN = atoi(argv[1]);
    }

    if (argc == 3) {
        tries = atoi(argv[2]);
    }

    if (tries < 1) {
        printf("Invalid tries supplied\n");
        exit(EXIT_FAILURE);
    }

    printf("Raspberry Pi wiringPi DHT22 reader\n") ;

    if (wiringPiSetup () == -1) {
        exit(EXIT_FAILURE) ;
    }
	
    if (setuid(getuid()) < 0) {
        perror("Dropping privileges failed\n");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        int currentTries = tries;
        while (read_dht22_dat() == 0 && currentTries--) {
            delay(1000); // wait 1sec to refresh
        }
        
        // Sleep for one minute
        delay(60000);
    }

    return 0;
}

