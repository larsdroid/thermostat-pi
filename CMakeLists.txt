# General: https://cmake.org/Wiki/CMake:How_To_Find_Libraries

cmake_minimum_required(VERSION 3.1.0)
project(thermostat-pi)
set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# TODO: -Wall ???
#       clang ???
#       EXECUTABLES: test_button test_database test_dht22 test_relay

# Math
set(LIBS m)

# libdl
set(LIBS ${LIBS} ${CMAKE_DL_LIBS})

# zlib
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
set(LIBS ${LIBS} ${ZLIB_LIBRARIES})

# OpenSSL
find_package(OpenSSL REQUIRED)
set(LIBS ${LIBS} ${OPENSSL_LIBRARIES})

# -pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(LIBS ${LIBS} Threads::Threads)

# -lwiringPi
find_package(Wiringpi REQUIRED)
include_directories(${WIRINGPI_INCLUDE_DIR})
set(LIBS ${LIBS} ${WIRINGPI_LIBRARIES})

# MariaDB or MySQL
find_package(MYSQL) # Not REQUIRED since MariaDB is just as good
if(${MYSQL_FOUND})
    include_directories(${MYSQL_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${MYSQL_LIBRARIES})
else()
    find_package(MariaDB REQUIRED)
    #set(MARIADB_CLIENT_FILENAME libmariadbclient.a)
    set(MARIADB_CLIENT_FILENAME libmariadb.a)
    include_directories(${MARIADB_INCLUDE_DIR})
    set(LIBS ${LIBS} ${MARIADB_LIBRARY_DIR}/${MARIADB_CLIENT_FILENAME})
endif()

include_directories(include)

file(GLOB SOURCES "src/*.c")

add_executable(thermostat ${SOURCES})

target_link_libraries(thermostat ${LIBS})
