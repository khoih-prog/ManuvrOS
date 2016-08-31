/* \mainpage ManuvrOS Documentation

        __  ___                             ____  _____
       /  |/  /___ _____  __  ___   _______/ __ \/ ___/
      / /|_/ / __ `/ __ \/ / / / | / / ___/ / / /\__ \  
     / /  / / /_/ / / / / /_/ /| |/ / /  / /_/ /___/ /  
    /_/  /_/\__,_/_/ /_/\__,_/ |___/_/   \____//____/   


![ManuvrOS](doc/3d-logo.png)

A cross-platform real-time, event-driven "Operating System" for asynchronous applications. It was intended to make it easier to write asynchronous, cross-platform IoT applications. Despite the name, ManuvrOS was intended to abstract threading and platform-related differences, and can be used on top of another operating system (as is the case on linux).

----------------------
#### What is in this repository:
**./doc**:  Location for generated documentation.

**./lib**:  Third-party libraries required for a project.

**./ManuvrOS**:  The ManuvrOS source code.

**./tests**:  Automated tests.

**./examples**:  Example projects.

**./downloadDeps.sh**   A script to download dependencies.


----------------------
### Building the test-bench
The test-bench consists of these files. It was meant to run on linux with possible Raspberry Pi extensions.
    FirmwareDefs.h
    main.cpp
    Makefile

##### Vanilla linux build (no i2c, TTY, or GPIO):

    make

##### The Raspi version of the test-bench:

    make BOARD=RASPI

##### You can also enable debugging features:

    make DEBUG=1

##### The doc can be built with (requires doxygen be installed):

    make docs

##### Automated tests:

    make tests

##### Various example programs:

    make examples


----------------------
### If you intend on using ManuvrOS in an Arduino-esque environment...
Arduino support has been shelved for the moment due to restrictions in their build system. It may make a re-appearance in the future.


----------------------
#### High-level TODO list
General issues and support goals are being tracked in Trello.
https://trello.com/b/3COZhcRs


----------------------
### License
Original code is Apache 2.0.

Code adapted from others' work inherits their license terms, which were preserved in the commentary where it applies.

----------------------
#### Cred:
The ASCII art in this file was generated by [this most-excellent tool](http://patorjk.com/software/taag).

Some of the drivers are adaptations from other's open code. This is noted in each specific class so derived.

*/
