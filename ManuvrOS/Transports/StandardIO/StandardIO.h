/*
File:   StandardIO.h
Author: J. Ian Lindsay
Date:   2016.07.23

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This driver allows us to treat an STDIO set under linux as if it were
  any other transport. It should be extendable to any situation where
  there exists a physical UI on the platform, and it may therefore be
  renamed and further extended in the future.
*/


#ifndef __MANUVR_STANDARD_IO_H__
#define __MANUVR_STANDARD_IO_H__

#if defined(__MANUVR_LINUX)
  #include <fcntl.h>
  #include <sys/signal.h>
  #include <fstream>
  #include <iostream>
  #include <termios.h>

  #include <cstdio>
  #include <stdlib.h>
  #include <unistd.h>
#else
  // Unsupported platform.
#endif

#include "../ManuvrXport.h"


class StandardIO : public ManuvrXport {
  public:
    StandardIO();
    ~StandardIO();

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(StringBuilder*, int8_t mm);

    /* Overrides from EventReceiver */
    int8_t attached();
    void printDebug(StringBuilder *);
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);

    /* Overrides from ManuvrXport */
    int8_t connect();
    int8_t disconnect();
    int8_t listen();
    int8_t reset();

    int8_t read_port();


  private:
};

#endif   // __MANUVR_STANDARD_IO_H__
