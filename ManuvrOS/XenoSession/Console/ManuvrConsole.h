/*
File:   ManuvrConsole.h
Author: J. Ian Lindsay
Date:   2016.05.28

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


This class represents a console session with the running Kernel. This simple
  role is done with this amount of complexity to avoid the necessity of
  designating a transport or a serial port specifically for this task, and to
  accomodate flexibility WRT logging.

If you want this feature, you must define MANUVR_CONSOLE_SUPPORT in the
  firmware defs file, or pass it into the build. Logging support will remain
  independently.
*/

#ifndef __MANUVR_CONSOLE_SESS_H__
#define __MANUVR_CONSOLE_SESS_H__

#include "ConsoleInterface.h"
#include "../XenoSession.h"


class ManuvrConsole : public EventReceiver, public BufferPipe, public ConsoleInterface {
  public:
    ManuvrConsole(BufferPipe*);
    ~ManuvrConsole();

    /* Override from BufferPipe. */
    const char* pipeName();
    virtual int8_t toCounterparty(ManuvrPipeSignal, void*);
    virtual int8_t toCounterparty(StringBuilder* buf, int8_t mm);
    virtual int8_t fromCounterparty(ManuvrPipeSignal, void*);
    virtual int8_t fromCounterparty(StringBuilder* buf, int8_t mm);

    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    int8_t attached();
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);

    /* Overrides from ConsoleInterface */
    uint consoleGetCmds(ConsoleCommand**);
    inline const char* consoleName() { return getReceiverName();  };
    void consoleCmdProc(StringBuilder* input);



  private:
    /*
    * A buffer for holding inbound stream until enough has arrived to parse. This eliminates
    *   the need for the transport to care about how much data we consumed versus left in its buffer.
    */
    StringBuilder session_buffer;
    StringBuilder _log_accumulator;
    ConsoleInterface* _current_console = this;
    bool _local_echo = false;  // Should input be echoed back to the console?
    bool _relay_all  = false;  // Relay log from everywhere, rather than just actions we provoke.

    int8_t _route_console_input(StringBuilder*);
    void change_active_console_interface(const char*);
    void change_active_console_interface(int);
};


#endif  // __MANUVR_CONSOLE_SESS_H__
