#!/bin/bash

gcc -std=c11 -lwiringPi -o dht22 dht22.c $(mariadb_config --libs --include --cflags)

