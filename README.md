Raspberry Pi Thermostat
==
This repository contains the source code of a Raspberry Pi thermostat written in C.

Dependencies
--
* POSIX Threads
* WiringPi
* MariaDB or MySQL (only tested with MariaDB)
* zlib
* OpenSSL
* libdl

Building
--
On top of the dependencies listed above, the cmake utility is required to build the code.  
Enter the following commands from within the project's root directory to start the build:  
`cmake .  
make`
