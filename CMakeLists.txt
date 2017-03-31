cmake_minimum_required(VERSION 3.1.0)
project(thermostat-pi)
set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# TODO: -Wall ???
#       clang ???

# -pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(GENERIC_LIBS Threads::Threads)

# -lwiringPi
find_package(Wiringpi REQUIRED)
include_directories(${WIRINGPI_INCLUDE_DIR})
set(GENERIC_LIBS ${GENERIC_LIBS} ${WIRINGPI_LIBRARIES})

# Math
set(DBLIBS m)

# libdl
set(DBLIBS ${DBLIBS} ${CMAKE_DL_LIBS})

# zlib
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
set(DBLIBS ${DBLIBS} ${ZLIB_LIBRARIES})

# OpenSSL
find_package(OpenSSL REQUIRED)
set(DBLIBS ${DBLIBS} ${OPENSSL_LIBRARIES})

# MariaDB or MySQL
find_package(MYSQL) # Not REQUIRED since MariaDB is just as good
if(${MYSQL_FOUND})
    include_directories(${MYSQL_INCLUDE_DIRS})
    set(DBLIBS ${DBLIBS} ${MYSQL_LIBRARIES})
else()
    find_package(MariaDB REQUIRED)
    #set(MARIADB_CLIENT_FILENAME libmariadbclient.a)
    set(MARIADB_CLIENT_FILENAME libmariadb.a)
    include_directories(${MARIADB_INCLUDE_DIR})
    set(DBLIBS ${DBLIBS} ${MARIADB_LIBRARY_DIR}/${MARIADB_CLIENT_FILENAME})
endif()

include_directories(include)

set(THERMOSTAT_SOURCES src/thermostat.c src/ini_file.c src/util.c src/display.c src/lcd.c src/db.c src/dht22.c src/mysql_util.c)
set(TEST_INI_FILE_SOURCES src/test_ini_file.c src/ini_file.c)
set(TEST_THREADS_SOURCES src/test_threads.c)
set(TEST_DISPLAY_SOURCES src/test_display.c src/lcd.c src/ini_file.c)
set(TEST_DATABASE_SOURCES src/test_database.c src/db.c src/ini_file.c src/mysql_util.c src/util.c)
set(TEST_BUTTON_SOURCES src/test_button.c src/ini_file.c src/util.c)
set(TEST_DHT22_SOURCES src/test_dht22.c src/ini_file.c src/dht22.c)
set(TEST_RELAY_SOURCES src/test_relay.c src/ini_file.c src/util.c)

add_executable(thermostat ${THERMOSTAT_SOURCES})
add_executable(test_ini_file ${TEST_INI_FILE_SOURCES})
add_executable(test_threads ${TEST_THREADS_SOURCES})
add_executable(test_display ${TEST_DISPLAY_SOURCES})
add_executable(test_database ${TEST_DATABASE_SOURCES})
add_executable(test_button ${TEST_BUTTON_SOURCES})
add_executable(test_dht22 ${TEST_DHT22_SOURCES})
add_executable(test_relay ${TEST_RELAY_SOURCES})

target_link_libraries(thermostat ${DBLIBS} ${GENERIC_LIBS})
# test_ini_file: NONE?
target_link_libraries(test_threads ${GENERIC_LIBS})
target_link_libraries(test_display ${GENERIC_LIBS})
target_link_libraries(test_database ${DBLIBS} ${GENERIC_LIBS})
target_link_libraries(test_button ${GENERIC_LIBS})
target_link_libraries(test_dht22 ${GENERIC_LIBS})
target_link_libraries(test_relay ${GENERIC_LIBS})
