CC=clang
COMMON_FLAGS=-std=c11 -Wall
CFLAGS=$(COMMON_FLAGS) -c
LFLAGS=$(COMMON_FLAGS) -lwiringPi -pthread

# MariaDB linking flags
ifneq ($(wildcard /usr/bin/mariadb_config),)
	MLFLAGS=$(shell mariadb_config --libs)
else
	MLFLAGS=$(shell mysql_config --libs)
endif

# MariaDB include flags
ifneq ($(wildcard /usr/bin/mariadb_config),)
	MCFLAGS=$(shell mariadb_config --cflags)
else
	MCFLAGS=$(shell mysql_config --cflags)
endif

all: test_ini_file test_display test_database thermostat test_button test_dht22 test_relay

test_ini_file: test_ini_file.o ini_file.o
	$(CC) $(LFLAGS) -o test_ini_file test_ini_file.o ini_file.o

test_threads: test_threads.o
	$(CC) $(LFLAGS) -o test_threads test_threads.o

test_display: test_display.o lcd.o ini_file.o
	$(CC) $(LFLAGS) -o test_display test_display.o lcd.o ini_file.o

test_database: test_database.o db.o ini_file.o mysql_util.o util.o
	$(CC) $(LFLAGS) $(MLFLAGS) -o test_database test_database.o db.o ini_file.o mysql_util.o util.o

thermostat: thermostat.o ini_file.o util.o display.o lcd.o db.o dht22.o mysql_util.o
	$(CC) $(LFLAGS) $(MLFLAGS) -o thermostat thermostat.o ini_file.o util.o display.o lcd.o db.o dht22.o mysql_util.o

test_button: test_button.o ini_file.o util.o
	$(CC) $(LFLAGS) -o test_button test_button.o ini_file.o util.o

test_dht22: test_dht22.o ini_file.o dht22.o
	$(CC) $(LFLAGS) -o test_dht22 test_dht22.o ini_file.o dht22.o

test_relay: test_relay.o ini_file.o util.o
	$(CC) $(LFLAGS) -o test_relay test_relay.o ini_file.o util.o

test_ini_file.o: test_ini_file.c ini_file.h
	$(CC) $(CFLAGS) test_ini_file.c

test_threads.o: test_threads.c
	$(CC) $(CFLAGS) test_threads.c

test_display.o: test_display.c lcd.h ini_file.h
	$(CC) $(CFLAGS) test_display.c

test_database.o: test_database.c ini_file.h db.h util.h
	$(CC) $(CFLAGS) $(MCFLAGS) test_database.c

thermostat.o: thermostat.c ini_file.h util.h display.h dht22.h
	$(CC) $(CFLAGS) thermostat.c

test_button.o: test_button.c ini_file.h util.h
	$(CC) $(CFLAGS) test_button.c

test_dht22.o: test_dht22.c ini_file.h dht22.h
	$(CC) $(CFLAGS) test_dht22.c

test_relay.o: test_relay.c ini_file.h util.h
	$(CC) $(CFLAGS) test_relay.c

ini_file.o: ini_file.c ini_file.h
	$(CC) $(CFLAGS) ini_file.c

display.o: display.c display.h ini_file.h lcd.h dht22.h
	$(CC) $(CFLAGS) display.c

dht22.o: dht22.c dht22.h ini_file.h
	$(CC) $(CFLAGS) dht22.c

util.o: util.c util.h
	$(CC) $(CFLAGS) util.c

lcd.o: lcd.c lcd.h
	$(CC) $(CFLAGS) lcd.c

db.o: db.c db.h mysql_util.h util.h
	$(CC) $(CFLAGS) $(MCFLAGS) db.c

mysql_util.o: mysql_util.c mysql_util.h ini_file.h util.h
	$(CC) $(CFLAGS) $(MCFLAGS) mysql_util.c

clean:
	rm -f thermostat test_button test_database test_dht22 test_relay *.o *.gch
