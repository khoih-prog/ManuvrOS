/*
File:   XenoSession.cpp
Author: J. Ian Lindsay
Date:   2014.11.20

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


XenoSession is the class that manages dialog with other systems via some
  transport (IRDa, Bluetooth, USB VCP, etc).
     ---J. Ian Lindsay
*/

#include "XenoSession.h"
#include <Kernel.h>


// TODO: This is temporary until the session is fully-abstracted.
#include "Manuvr/ManuvrSession.h"



/****************************************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here. Initializers first, functions second.
****************************************************************************************************/

/**
* Logging support fxn.
*/
const char* XenoSession::getSessionStateString(uint16_t state_code) {
  switch (state_code & 0x000F) {
    case XENOSESSION_STATE_UNINITIALIZED:    return "UNINITIALIZED";
    case XENOSESSION_STATE_PENDING_SETUP:    return "PENDING_SETUP";
    case XENOSESSION_STATE_PENDING_AUTH:     return "PENDING_AUTH";
    case XENOSESSION_STATE_ESTABLISHED:      return "ESTABLISHED";
    case XENOSESSION_STATE_PENDING_HANGUP:   return "HANGUP IN PROGRESS";
    case XENOSESSION_STATE_HUNGUP:           return "HUNGUP";
    case XENOSESSION_STATE_DISCONNECTED:     return "DISCONNECTED";
    default:                                 return "<UNKNOWN SESSION STATE>";
  }
}


/****************************************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
****************************************************************************************************/

/**
* When a connectable class gets a connection, we get instantiated to handle the protocol...
*
* @param   ManuvrXport* All sessions must have one (and only one) transport.
*/
XenoSession::XenoSession(ManuvrXport* _xport) {
  __class_initializer();

  owner = _xport;
  session_state             = XENOSESSION_STATE_UNINITIALIZED;
  session_last_state        = XENOSESSION_STATE_UNINITIALIZED;
  sequential_parse_failures = 0;
  sequential_ack_failures   = 0;
  working                   = NULL;

  MAX_PARSE_FAILURES  = 3;  // How many failures-to-parse should we tolerate before SYNCing?
  MAX_ACK_FAILURES    = 3;  // How many failures-to-ACK should we tolerate before SYNCing?
}


/**
* Unlike many of the other EventReceivers, THIS one needs to be able to be torn down.
*/
XenoSession::~XenoSession() {
  __kernel->unsubscribe((EventReceiver*) this);  // Unsubscribe

  purgeInbound();  // Need to do careful checks in here for open comm loops.
  purgeOutbound(); // Need to do careful checks in here for open comm loops.

  Kernel::raiseEvent(MANUVR_MSG_SESS_HANGUP, NULL);
}



/**
* Tell the session to relay the given message code.
* We have a list of blacklisted codes that the session is not allowed to subscribe to.
*   The reason for this is to avoid an Event loop. The blacklist might be removed once
*   the EXPORT flag in the Message legend is being respected.
*
* @param   uint16_t The message type code to subscribe the session to.
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::tapMessageType(uint16_t code) {
  switch (code) {
    case MANUVR_MSG_SESS_ORIGINATE_MSG:
    case MANUVR_MSG_SESS_DUMP_DEBUG:
//    case MANUVR_MSG_SESS_HANGUP:
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 3) local_log.concatf("0x%08x tapMessageType() tried to tap a blacklisted code (%d).\n", (uint32_t) this, code);
      #endif
      return -1;
  }

  MessageTypeDef* temp_msg_def = (MessageTypeDef*) ManuvrMsg::lookupMsgDefByCode(code);
  if (!msg_relay_list.contains(temp_msg_def)) {
    msg_relay_list.insert(temp_msg_def);
  }
  return 0;
}


/**
* Tell the session not to relay the given message code.
*
* @param   uint16_t The message type code to unsubscribe the session from.
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::untapMessageType(uint16_t code) {
  msg_relay_list.remove((MessageTypeDef*) ManuvrMsg::lookupMsgDefByCode(code));
  return 0;
}


/**
* Unsubscribes the session from all optional messages.
* Whatever messages this session was watching for, it will not be after this call.
*
* @return  nonzero if there was a problem.
*/
int8_t XenoSession::untapAll() {
  msg_relay_list.clear();
  return 0;
}


/**
* Mark a given message as complete and ready for reap.
*
* @param   uint16_t The message unique_id to mark completed.
* @return  0 if nothing done, negative if there was a problem.
*/
int8_t XenoSession::markMessageComplete(uint16_t target_id) {
  XenoMessage *working_xeno;
  for (int i = 0; i < outbound_messages.size(); i++) {
    working_xeno = outbound_messages.get(i);
    if (target_id == working_xeno->uniqueId()) {
      switch (working_xeno->getState()) {
        case XENO_MSG_PROC_STATE_AWAITING_REAP:
          outbound_messages.remove(working_xeno);
          XenoMessage::reclaimPreallocation(working_xeno);
          return 1;
      }
    }
  }
  return 0;
}




/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀
*
* These are overrides from EventReceiver interface...
****************************************************************************************************/

/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t XenoSession::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    default:
      break;
  }

  return return_value;
}


/*
* This is the override from EventReceiver, but there is a bit of a twist this time.
* Following the normal processing of the incoming event, this class compares it against
*   a list of events that it has been instructed to relay to the counterparty. If the event
*   meets the relay criteria, we serialize it and send it to the transport that we are bound to.
*/
int8_t XenoSession::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->event_code) {
    /* General system events */
    case MANUVR_MSG_BT_CONNECTION_LOST:
      session_state = XENOSESSION_STATE_DISCONNECTED;
      //msg_relay_list.clear();
      purgeInbound();
      purgeOutbound();
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 3) local_log.concatf("0x%08x Session is now in state %s.\n", (uint32_t) this, getSessionStateString(getPhase()));
      #endif
      return_value++;
      break;

    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_SESS_HANGUP:
      {
        int out_purge = purgeOutbound();
        int in_purge  = purgeInbound();
        #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 5) local_log.concatf("0x%08x Purged (%d) msgs from outbound and (%d) from inbound.\n", (uint32_t) this, out_purge, in_purge);
        #endif
      }
      return_value++;
      break;

    case MANUVR_MSG_XPORT_RECEIVE:
      {
        StringBuilder* buf;
        if (0 == active_event->getArgAs(&buf)) {
          bin_stream_rx(buf->string(), buf->length());
        }
      }
      return_value++;
      break;

    case MANUVR_MSG_SESS_DUMP_DEBUG:
      printDebug(&local_log);
      return_value++;
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  /* We don't want to resonate... Don't react to Events that have us as the originator. */
  if (active_event->originator != (EventReceiver*) this) {
    if ((XENO_SESSION_IGNORE_NON_EXPORTABLES) && (active_event->isExportable())) {
      /* This is the block that allows the counterparty to intercept events of its choosing. */
      if (msg_relay_list.contains(active_event->getMsgDef())) {
        // If we are in this block, it means we need to serialize the event and send it.
        sendEvent(active_event);
        return_value++;
      }
    }
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}




/****************************************************************************************************
* Functions for managing dialogs and message queues.                                                *
****************************************************************************************************/

/**
* Empties the outbound message queue (those bytes designated for the transport).
*
* @return  int The number of outbound messages that were purged.
*/
int XenoSession::purgeOutbound() {
  int return_value = outbound_messages.size();
  XenoMessage* temp;
  while (outbound_messages.hasNext()) {
    temp = outbound_messages.get();
    #ifdef __MANUVR_DEBUG
    if (getVerbosity() > 6) {
      local_log.concatf("\nSession 0x%08x Destroying outbound msg:\n", (uint32_t) this);
      temp->printDebug(&local_log);
      Kernel::log(&local_log);
    }
    #endif
    delete temp;
    outbound_messages.remove();
  }
  return return_value;
}


/**
* Empties the inbound message queue (those bytes from the transport that we need to proc).
*
* @return  int The number of inbound messages that were purged.
*/
int XenoSession::purgeInbound() {
  int return_value = inbound_messages.size();
  XenoMessage* temp;
  while (inbound_messages.hasNext()) {
    temp = inbound_messages.get();
    #ifdef __MANUVR_DEBUG
    if (getVerbosity() > 6) {
      local_log.concatf("\nSession 0x%08x Destroying inbound msg:\n", (uint32_t) this);
      temp->printDebug(&local_log);
      Kernel::log(&local_log);
    }
    #endif
    delete temp;
    inbound_messages.remove();
  }
  session_buffer.clear();   // Also purge whatever hanging RX buffer we may have had.
  return return_value;
}



/**
* We may decide to send a no-argument packet that demands acknowledgement so that we can...
*  1. Unambiguously recover from a desync state without resorting to timers.
*  2. Periodically ping the counterparty to ensure that we have not disconnected.
*
* The keep-alive system is not handled in the transport because it is part of the protocol.
* A transport might have its own link-layer-appropriate keep-alive mechanism, which can be
*   used in-place of the KA at this (Session) layer. In such case, the Transport class would
*   carry configuration flags/members that co-ordinate with this class so that the Session
*   doesn't feel the need to use case (2) given above.
*               ---J. Ian Lindsay   Tue Aug 04 23:12:55 MST 2015
*/
int8_t XenoSession::sendKeepAlive() {
  if (owner->connected()) {
    ManuvrRunnable* ka_event = Kernel::returnEvent(MANUVR_MSG_SYNC_KEEPALIVE);
    sendEvent(ka_event);

    //ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_XPORT_SEND);
    //event->specific_target = owner;  //   event to be the transport that instantiated us.
    //raiseEvent(event);
  }
  return 0;
}


/**
* Passing an Event into this fxn will cause the Event to be serialized and sent to our counter-party.
* This is the point at which choices are made about what happens to the event's life-cycle.
*/
int8_t XenoSession::sendEvent(ManuvrRunnable *active_event) {
  XenoMessage* nu_outbound_msg = XenoMessage::fetchPreallocation(this);
  nu_outbound_msg->provideEvent(active_event);

  StringBuilder buf;
  if (nu_outbound_msg->serialize(&buf) > 0) {
    owner->sendBuffer(&buf);
  }

  if (nu_outbound_msg->expectsACK()) {
    outbound_messages.insert(nu_outbound_msg);
  }

  // We are about to pass a message across the transport.
  //ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_XPORT_SEND);
  //event->originator      = this;   // We want the callback and the only receiver of this
  //event->specific_target = owner;  //   event to be the transport that instantiated us.
  //raiseEvent(event);
  return 0;
}



// At this point, we can take it for granted that this message contains a valid event.
// This is only for processing inbound messages.
int8_t XenoSession::take_message() {
  XenoMessage* nu_xm = working;
  working = NULL;                   // ...make space for the next message.

  switch (nu_xm->event->event_code) {
    case MANUVR_MSG_REPLY:
      break;

    case MANUVR_MSG_SYNC_KEEPALIVE:
      break;

    default:
      if (nu_xm->expectsACK()) {
        inbound_messages.insert(nu_xm);   // ...drop the new message into the inbound message queue.
      }
      break;
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return 0;
}




/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄   ▄         ▄  ▄▄▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀
* ▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌
* ▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░▌       ▐░▌▐░▌ ▄▄▄▄▄▄▄▄
* ▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░▌▐░░░░░░░░▌
* ▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░▌ ▀▀▀▀▀▀█░▌
* ▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌
* ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀
****************************************************************************************************/

/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void XenoSession::printDebug(StringBuilder *output) {
  if (NULL == output) return;
  EventReceiver::printDebug(output);

  output->concatf("-- Session ID           0x%08x\n", (uint32_t) this);
  output->concatf("-- Session state        %s\n", getSessionStateString(getPhase()));
  output->concatf("-- seq parse failures   %d\n", sequential_parse_failures);
  output->concatf("-- seq_ack_failures     %d\n--\n", sequential_ack_failures);
  output->concatf("-- _heap_instantiations %u\n", (unsigned long) XenoMessage::_heap_instantiations);
  output->concatf("-- _heap_frees          %u\n", (unsigned long) XenoMessage::_heap_freeds);

  int ses_buf_len = session_buffer.length();
  if (ses_buf_len > 0) {
    output->concatf("\n-- Session Buffer (%d bytes) --------------------------\n", ses_buf_len);
    for (int i = 0; i < ses_buf_len; i++) {
      output->concatf("0x%02x ", *(session_buffer.string() + i));
    }
    output->concat("\n\n");
  }

  output->concat("\n-- Listening for the following event codes:\n");
  int x = msg_relay_list.size();
  for (int i = 0; i < x; i++) {
    output->concatf("\t%s\n", msg_relay_list.get(i)->debug_label);
  }

  x = outbound_messages.size();
  if (x > 0) {
    output->concatf("\n-- Outbound Queue %d total, showing top %d ------------\n", x, XENO_SESSION_MAX_QUEUE_PRINT);
    for (int i = 0; i < min(x, XENO_SESSION_MAX_QUEUE_PRINT); i++) {
        outbound_messages.get(i)->printDebug(output);
    }
  }

  x = inbound_messages.size();
  if (x > 0) {
    output->concatf("\n-- Inbound Queue %d total, showing top %d -------------\n", x, XENO_SESSION_MAX_QUEUE_PRINT);
    for (int i = 0; i < min(x, XENO_SESSION_MAX_QUEUE_PRINT); i++) {
      inbound_messages.get(i)->printDebug(output);
    }
  }

  if (NULL != working) {
    output->concat("\n-- XenoMessage in process  ----------------------------\n");
    working->printDebug(output);
  }
}


void XenoSession::procDirectDebugInstruction(StringBuilder *input) {
  uint8_t temp_byte = 0;

  char* str = input->position(0);

  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  switch (*(str)) {
    case 'S':  // Send a mess of sync packets.
      //sync_event.alterScheduleRecurrence(XENOSESSION_INITIAL_SYNC_COUNT);
      //sync_event.enableSchedule(true);
      break;
    case 'i':  // Send a mess of sync packets.
      if (1 == temp_byte) {
      }
      else if (2 == temp_byte) {
      }
      else {
        printDebug(&local_log);
      }
      break;
    case 'q':  // Manual message queue purge.
      purgeOutbound();
      purgeInbound();
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}
