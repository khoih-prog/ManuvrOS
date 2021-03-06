/*
File:   ManuvrGPS.cpp
Author: J. Ian Lindsay
Date:   2016.06.29

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


This class was an adaption to Manuvr based on Kosma Moczek's minmea.
https://github.com/kosma/minmea
*/

/*
* Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
* This program is free software. It comes without any warranty, to the extent
* permitted by applicable law. You can redistribute it and/or modify it under
* the terms of the Do What The Fuck You Want To Public License, Version 2, as
* published by Sam Hocevar. See the COPYING file for more details.
*/

#include "ManuvrGPS.h"

#if defined(CONFIG_MANUVR_GPS_PIPE)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#ifdef __cplusplus
}
#endif


const DatumDef datum_defs[] = {
  {
    .desc    = "Speed",
    .units   = COMMON_UNITS_KM_PER_HOUR,
    .type_id = TCode::DOUBLE,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Latitude",
    .units   = COMMON_UNITS_DEGREES,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Longitude",
    .units   = COMMON_UNITS_DEGREES,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Altitude",
    .units   = COMMON_UNITS_METERS,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};



/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

static inline bool minmea_isfield(char c) {
  return isprint((unsigned char) c) && c != ',' && c != '*';
}

static int hex2int(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

ManuvrGPS::ManuvrGPS() : BufferPipe(), SensorWrapper("GPS") {
  _bp_set_flag(BPIPE_FLAG_IS_TERMINUS | BPIPE_FLAG_IS_BUFFERED, true);
  define_datum(&datum_defs[0]);
  define_datum(&datum_defs[1]);
  define_datum(&datum_defs[2]);
  define_datum(&datum_defs[3]);
  isActive(true);
  isCalibrated(true);
}


ManuvrGPS::ManuvrGPS(BufferPipe* src) : ManuvrGPS() {
  setNear(src);
}


ManuvrGPS::~ManuvrGPS() {
  _accumulator.clear();
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
const char* ManuvrGPS::pipeName() { return "ManuvrGPS"; }

/**
* Pass a signal to the counterparty.
*
* Data referenced by _args should be assumed to be on the stack of the caller.
*
* @param   _sig   The signal.
* @param   _args  Optional argument pointer.
* @return  Negative on error. Zero on success.
*/
int8_t ManuvrGPS::fromCounterparty(ManuvrPipeSignal _sig, void* _args) {
  #if defined(MANUVR_PIPE_DEBUG)
    StringBuilder local_log;
    local_log.concatf("%s --sig--> %s: %s\n", (haveNear() ? near()->pipeName() : "ORIG"), pipeName(), signalString(_sig));
    Kernel::log(&local_log);
  #endif
  switch (_sig) {
    case ManuvrPipeSignal::XPORT_CONNECT:
    case ManuvrPipeSignal::XPORT_DISCONNECT:
      // If we lose or gain a connection, wipe the buffer.
      _accumulator.clear();
      return 1;

    case ManuvrPipeSignal::FAR_SIDE_DETACH:   // The far side is detaching.
    case ManuvrPipeSignal::NEAR_SIDE_DETACH:   // The near side is detaching.
    case ManuvrPipeSignal::FAR_SIDE_ATTACH:
    case ManuvrPipeSignal::NEAR_SIDE_ATTACH:
    case ManuvrPipeSignal::UNDEF:
    default:
      break;
  }
  return BufferPipe::fromCounterparty(_sig, _args);
}


/**
* Inward toward the transport.
* Doesn't make sense for GPS.
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrGPS::toCounterparty(StringBuilder* buf, int8_t mm) {
  Kernel::log("ManuvrGPS is Rx-only.\n");
  buf->clear();
  return MEM_MGMT_RESPONSIBLE_ERROR;
}


/**
* Outward toward the application (or into the accumulator).
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrGPS::fromCounterparty(StringBuilder* buf, int8_t mm) {
  _accumulator.concatHandoff(buf);
  if (MINMEA_MAX_LENGTH < _accumulator.length()) {
    _attempt_parse();
  }
  return MEM_MGMT_RESPONSIBLE_BEARER;   // We take responsibility.
}


/*******************************************************************************
*  ,-.                      ,   .
* (   `                     | . |
*  `-.  ,-. ;-. ,-. ,-. ;-. | ) ) ;-. ,-: ;-. ;-. ,-. ;-.
* .   ) |-' | | `-. | | |   |/|/  |   | | | | | | |-' |
*  `-'  `-' ' ' `-' `-' '   ' '   '   `-` |-' |-' `-' '
******************************************|***|********************************/

SensorError ManuvrGPS::init() {
  _accumulator.clear();
  return haveNear() ? SensorError::NO_ERROR : SensorError::BUS_ERROR;
}


SensorError ManuvrGPS::readSensor() {
  return haveNear() ? SensorError::NO_ERROR : SensorError::BUS_ERROR;
}


SensorError ManuvrGPS::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError ManuvrGPS::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}



/*******************************************************************************
* GPS-specific functions                                                       *
*******************************************************************************/

const char* ManuvrGPS::_get_string_by_sentence_id(enum minmea_sentence_id id) {
  switch (id) {
    case MINMEA_UNKNOWN:      return "UNKNOWN";
    case MINMEA_SENTENCE_RMC: return "RMC";
    case MINMEA_SENTENCE_GGA: return "GGA";
    case MINMEA_SENTENCE_GSA: return "GSA";
    case MINMEA_SENTENCE_GLL: return "GLL";
    case MINMEA_SENTENCE_GST: return "GST";
    case MINMEA_SENTENCE_GSV: return "GSV";
    case MINMEA_SENTENCE_VTG: return "VTG";
    default:
      break;
  }
  return "xxx";
}


enum minmea_sentence_id ManuvrGPS::_sentence_id(const char *sentence, bool strict) {
  if (_check(sentence, strict)) {
    char type[6];
    if (_scan(sentence, "t", type)) {
      uint32_t int_sent_code = (type[2] << 16) + (type[3] << 8) + (type[4]);
      switch (int_sent_code) {
        case MINMEA_INT_SENTENCE_CODE_RMC:  return MINMEA_SENTENCE_RMC;
        case MINMEA_INT_SENTENCE_CODE_GGA:  return MINMEA_SENTENCE_GGA;
        case MINMEA_INT_SENTENCE_CODE_GSA:  return MINMEA_SENTENCE_GSA;
        case MINMEA_INT_SENTENCE_CODE_GLL:  return MINMEA_SENTENCE_GLL;
        case MINMEA_INT_SENTENCE_CODE_GST:  return MINMEA_SENTENCE_GST;
        case MINMEA_INT_SENTENCE_CODE_GSV:  return MINMEA_SENTENCE_GSV;
        case MINMEA_INT_SENTENCE_CODE_VTG:  return MINMEA_SENTENCE_VTG;
        default:                            return MINMEA_UNKNOWN;
      }
    }
  }
  return MINMEA_INVALID;
}


bool ManuvrGPS::_attempt_parse() {
  if (_accumulator.split("\n") == 0) return false;
  char* line = nullptr;
  bool local_success = false;
  StringBuilder _log;

  while (_accumulator.count() > 1) {
    line = _accumulator.position(0);
    enum minmea_sentence_id id = _sentence_id(line, false);
    switch (id) {
      case MINMEA_SENTENCE_GSA:
        {
          struct minmea_sentence_gsa frame;
          if (_parse_gsa(&frame, line)) {
            local_success = true;
          }
        }
        break;
      case MINMEA_SENTENCE_GLL:
        {
          struct minmea_sentence_gll frame;
          if (_parse_gll(&frame, line)) {
            local_success = true;
          }
        }
        break;
      case MINMEA_SENTENCE_RMC:
        {
          struct minmea_sentence_rmc frame;
          if (_parse_rmc(&frame, line)) {
            local_success = true;
            updateDatum(1, minmea_tocoord(&frame.latitude));
            updateDatum(2, minmea_tocoord(&frame.longitude));
              // _log.concatf("$xxRMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\n",
              //         frame.latitude.value, frame.latitude.scale,
              //         frame.longitude.value, frame.longitude.scale,
              //         frame.speed.value, frame.speed.scale);
              // _log.concatf("$xxRMC fixed-point coordinates and speed scaled to three decimal places: (%d,%d) %d\n",
              //         minmea_rescale(&frame.latitude, 1000),
              //         minmea_rescale(&frame.longitude, 1000),
              //         minmea_rescale(&frame.speed, 1000));
              // _log.concatf("$xxRMC floating point degree coordinates and speed: (%f,%f) %f\n",
              //         (double) minmea_tocoord(&frame.latitude),
              //         (double) minmea_tocoord(&frame.longitude),
              //         (double) minmea_tofloat(&frame.speed));
          }
        }
        break;
      case MINMEA_SENTENCE_GGA:
        {
          struct minmea_sentence_gga frame;
          if (_parse_gga(&frame, line)) {
            local_success = true;
            //_log.concatf("$xxGGA: fix quality: %d\n", frame.fix_quality);
          }
        }
        break;
      case MINMEA_SENTENCE_GST:
        {
          struct minmea_sentence_gst frame;
          if (_parse_gst(&frame, line)) {
            local_success = true;
              // _log.concatf("$xxGST: raw latitude,longitude and altitude error deviation: (%d/%d,%d/%d,%d/%d)\n",
              //         frame.latitude_error_deviation.value, frame.latitude_error_deviation.scale,
              //         frame.longitude_error_deviation.value, frame.longitude_error_deviation.scale,
              //         frame.altitude_error_deviation.value, frame.altitude_error_deviation.scale);
              // _log.concatf("$xxGST fixed point latitude,longitude and altitude error deviation"
              //        " scaled to one decimal place: (%d,%d,%d)\n",
              //         minmea_rescale(&frame.latitude_error_deviation, 10),
              //         minmea_rescale(&frame.longitude_error_deviation, 10),
              //         minmea_rescale(&frame.altitude_error_deviation, 10));
              // _log.concatf("$xxGST floating point degree latitude, longitude and altitude error deviation: (%f,%f,%f)",
              //         (double) minmea_tofloat(&frame.latitude_error_deviation),
              //         (double) minmea_tofloat(&frame.longitude_error_deviation),
              //         (double) minmea_tofloat(&frame.altitude_error_deviation));
          }
        }
        break;
      case MINMEA_SENTENCE_GSV:
        {
          struct minmea_sentence_gsv frame;
          if (_parse_gsv(&frame, line)) {
            local_success = true;
              // _log.concatf("$xxGSV: message %d of %d\n", frame.msg_nr, frame.total_msgs);
              // _log.concatf("$xxGSV: sattelites in view: %d\n", frame.total_sats);
              // for (int i = 0; i < 4; i++)
              //     _log.concatf("$xxGSV: sat nr %d, elevation: %d, azimuth: %d, snr: %d dbm\n",
              //         frame.sats[i].nr,
              //         frame.sats[i].elevation,
              //         frame.sats[i].azimuth,
              //         frame.sats[i].snr);
          }
        }
        break;
      case MINMEA_SENTENCE_VTG:
        {
          struct minmea_sentence_vtg frame;
          if (_parse_vtg(&frame, line)) {
            local_success = true;
            updateDatum(0, minmea_tofloat(&frame.speed_kph));

              // _log.concatf("$xxVTG: true track degrees = %f\n",
              //        (double) minmea_tofloat(&frame.true_track_degrees));
              // _log.concatf("        magnetic track degrees = %f\n",
              //        (double) minmea_tofloat(&frame.magnetic_track_degrees));
              // _log.concatf("        speed knots = %f\n",
              //         (double) minmea_tofloat(&frame.speed_knots));
              // _log.concatf("        speed kph = %f\n",
              //         (double) minmea_tofloat(&frame.speed_kph));
          }
        }
        break;
      case MINMEA_INVALID:
      default:
        break;
    }
    if (local_success) {
      _sentences_parsed++;
    }
    else {
      _sentences_rejected++;
      _log.concatf("$xx%s sentence is not parsed: \n%s\n", _get_string_by_sentence_id(id), (const char*) line);
    }
    _accumulator.drop_position(0);
  }
  Kernel::log(&_log);
  return true;
}


void ManuvrGPS::printDebug(StringBuilder* output) {
  BufferPipe::printDebug(output);
  output->concatf("\tSentences\n\t-------------\n\tParsed %u\n\tReject %u\n", _sentences_parsed, _sentences_rejected);

  if (_accumulator.length() > 0) {
    output->concatf("\n\taccumulator (%d bytes):  ", _accumulator.length());
    _accumulator.printDebug(output);
  }
}


uint8_t ManuvrGPS::_checksum(const char *sentence) {
  // Support senteces with or without the starting dollar sign.
  if (*sentence == '$') sentence++;
  uint8_t checksum = 0x00;

  // The optional checksum is an XOR of all bytes between "$" and "*".
  while (*sentence && *sentence != '*') checksum ^= *sentence++;
  return checksum;
}


bool ManuvrGPS::_check(const char *sentence, bool strict) {
  // Sequence length is limited.
  if (strlen(sentence) <= MINMEA_MAX_LENGTH + 3) {
    // A valid sentence starts with "$".
    if (*sentence++ == '$') {
      // The optional checksum is an XOR of all bytes between "$" and "*".
      uint8_t checksum = 0x00;
      while (*sentence && *sentence != '*' && isprint((unsigned char) *sentence)) {
        checksum ^= *sentence++;
      }
      if (*sentence == '*') {
        // If checksum is present, extract and compare it.
        sentence++;
        int upper = hex2int(*sentence++);
        if (upper != -1) {
          int lower = hex2int(*sentence++);
          if (lower != -1) {
            int expected = upper << 4 | lower;
            // Check for checksum mismatch.
            return (checksum == expected);
          }
        }
      }
      else if (*sentence && (!strict) && ((*sentence == '\n') || (*sentence == '\r'))) {
        // The only stuff allowed at this point is a newline, and then only if
        //   strict mode is false.
        return true;
      }
    }
  }
  return false;
}




/*******************************************************************************
* Undigested GPS functions                                                     *
*******************************************************************************/

bool ManuvrGPS::_scan(const char *sentence, const char *format, ...) {
  bool result = false;
  bool optional = false;
  va_list ap;
  va_start(ap, format);

  const char *field = sentence;
#define next_field() \
    do { \
        /* Progress to the next field. */ \
        while (minmea_isfield(*sentence)) \
            sentence++; \
        /* Make sure there is a field there. */ \
        if (*sentence == ',') { \
            sentence++; \
            field = sentence; \
        } else { \
            field = nullptr; \
        } \
    } while (0)

    while (*format) {
        char type = *format++;

        if (type == ';') {
            // All further fields are optional.
            optional = true;
            continue;
        }

        if (!field && !optional) {
            // Field requested but we ran out if input. Bail out.
            goto parse_error;
        }

        switch (type) {
            case 'c': { // Single character field (char).
                char value = '\0';

                if (field && minmea_isfield(*field))
                    value = *field;

                *va_arg(ap, char *) = value;
            } break;

            case 'd': { // Single character direction field (int).
                int value = 0;

                if (field && minmea_isfield(*field)) {
                    switch (*field) {
                        case 'N':
                        case 'E':
                            value = 1;
                            break;
                        case 'S':
                        case 'W':
                            value = -1;
                            break;
                        default:
                            goto parse_error;
                    }
                }

                *va_arg(ap, int *) = value;
            } break;

            case 'f': { // Fractional value with scale (struct minmea_float).
                int sign = 0;
                int_least32_t value = -1;
                int_least32_t scale = 0;

                if (field) {
                    while (minmea_isfield(*field)) {
                        if (*field == '+' && !sign && value == -1) {
                            sign = 1;
                        } else if (*field == '-' && !sign && value == -1) {
                            sign = -1;
                        } else if (isdigit((unsigned char) *field)) {
                            int digit = *field - '0';
                            if (value == -1)
                                value = 0;
                            if (value > (INT_LEAST32_MAX-digit) / 10) {
                                /* we ran out of bits, what do we do? */
                                if (scale) {
                                    /* truncate extra precision */
                                    break;
                                } else {
                                    /* integer overflow. bail out. */
                                    goto parse_error;
                                }
                            }
                            value = (10 * value) + digit;
                            if (scale)
                                scale *= 10;
                        } else if (*field == '.' && scale == 0) {
                            scale = 1;
                        } else if (*field == ' ') {
                            /* Allow spaces at the start of the field. Not NMEA
                             * conformant, but some modules do this. */
                            if (sign != 0 || value != -1 || scale != 0)
                                goto parse_error;
                        } else {
                            goto parse_error;
                        }
                        field++;
                    }
                }

                if ((sign || scale) && value == -1)
                    goto parse_error;

                if (value == -1) {
                    /* No digits were scanned. */
                    value = 0;
                    scale = 0;
                } else if (scale == 0) {
                    /* No decimal point. */
                    scale = 1;
                }
                if (sign)
                    value *= sign;

                *va_arg(ap, struct minmea_float *) = (struct minmea_float) {value, scale};
            } break;

            case 'i': { // Integer value, default 0 (int).
                int value = 0;

                if (field) {
                    char *endptr;
                    value = strtol(field, &endptr, 10);
                    if (minmea_isfield(*endptr))
                        goto parse_error;
                }

                *va_arg(ap, int *) = value;
            } break;

            case 's': { // String value (char *).
                char *buf = va_arg(ap, char *);

                if (field) {
                    while (minmea_isfield(*field))
                        *buf++ = *field++;
                }

                *buf = '\0';
            } break;

            case 't': { // NMEA talker+sentence identifier (char *).
                // This field is always mandatory.
                if (!field)
                    goto parse_error;

                if (field[0] != '$')
                    goto parse_error;
                for (int f=0; f<5; f++)
                    if (!minmea_isfield(field[1+f]))
                        goto parse_error;

                char *buf = va_arg(ap, char *);
                memcpy(buf, field+1, 5);
                buf[5] = '\0';
            } break;

            case 'D': { // Date (int, int, int), -1 if empty.
                struct minmea_date *date = va_arg(ap, struct minmea_date *);

                int d = -1, m = -1, y = -1;

                if (field && minmea_isfield(*field)) {
                    // Always six digits.
                    for (int f=0; f<6; f++)
                        if (!isdigit((unsigned char) field[f]))
                            goto parse_error;

                    char dArr[] = {field[0], field[1], '\0'};
                    char mArr[] = {field[2], field[3], '\0'};
                    char yArr[] = {field[4], field[5], '\0'};
                    d = strtol(dArr, nullptr, 10);
                    m = strtol(mArr, nullptr, 10);
                    y = strtol(yArr, nullptr, 10);
                }

                date->day = d;
                date->month = m;
                date->year = y;
            } break;

            case 'T': { // Time (int, int, int, int), -1 if empty.
                struct minmea_time *time_ = va_arg(ap, struct minmea_time *);

                int h = -1, i = -1, s = -1, u = -1;

                if (field && minmea_isfield(*field)) {
                    // Minimum required: integer time.
                    for (int f=0; f<6; f++)
                        if (!isdigit((unsigned char) field[f]))
                            goto parse_error;

                    char hArr[] = {field[0], field[1], '\0'};
                    char iArr[] = {field[2], field[3], '\0'};
                    char sArr[] = {field[4], field[5], '\0'};
                    h = strtol(hArr, nullptr, 10);
                    i = strtol(iArr, nullptr, 10);
                    s = strtol(sArr, nullptr, 10);
                    field += 6;

                    // Extra: fractional time. Saved as microseconds.
                    if (*field++ == '.') {
                        int value = 0;
                        int scale = 1000000;
                        while (isdigit((unsigned char) *field) && scale > 1) {
                            value = (value * 10) + (*field++ - '0');
                            scale /= 10;
                        }
                        u = value * scale;
                    } else {
                        u = 0;
                    }
                }

                time_->hours = h;
                time_->minutes = i;
                time_->seconds = s;
                time_->microseconds = u;
            } break;

            case '_': { // Ignore the field.
            } break;

            default: { // Unknown.
                goto parse_error;
            } break;
        }

        next_field();
    }

    result = true;

parse_error:
    va_end(ap);
    return result;
}


bool ManuvrGPS::_talker_id(char talker[3], const char *sentence) {
  char type[6];
  if (!_scan(sentence, "t", type)) {
    return false;
  }
  talker[0] = type[0];
  talker[1] = type[1];
  talker[2] = '\0';
  return true;
}



bool ManuvrGPS::_parse_rmc(struct minmea_sentence_rmc *frame, const char *sentence) {
  // $GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62
  char type[6];
  char validity;
  int latitude_direction;
  int longitude_direction;
  int variation_direction;
  if (!_scan(sentence, "tTcfdfdffDfd",
      type,
      &frame->time,
      &validity,
      &frame->latitude, &latitude_direction,
      &frame->longitude, &longitude_direction,
      &frame->speed,
      &frame->course,
      &frame->date,
      &frame->variation, &variation_direction)) {
    return false;
  }
  if (strcmp(type+2, "RMC")) {
    return false;
  }
  frame->valid = (validity == 'A');
  frame->latitude.value *= latitude_direction;
  frame->longitude.value *= longitude_direction;
  frame->variation.value *= variation_direction;
  return true;
}

bool ManuvrGPS::_parse_gga(struct minmea_sentence_gga *frame, const char *sentence) {
  // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
  char type[6];
  int latitude_direction;
  int longitude_direction;

  if (!_scan(sentence, "tTfdfdiiffcfci_",
      type,
      &frame->time,
      &frame->latitude, &latitude_direction,
      &frame->longitude, &longitude_direction,
      &frame->fix_quality,
      &frame->satellites_tracked,
      &frame->hdop,
      &frame->altitude, &frame->altitude_units,
      &frame->height, &frame->height_units,
      &frame->dgps_age)) {
    return false;
  }
  if (strcmp(type+2, "GGA")) {
    return false;
  }

  frame->latitude.value *= latitude_direction;
  frame->longitude.value *= longitude_direction;
  return true;
}

bool ManuvrGPS::_parse_gsa(struct minmea_sentence_gsa *frame, const char *sentence) {
    // $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
    char type[6];

    if (!_scan(sentence, "tciiiiiiiiiiiiifff",
            type,
            &frame->mode,
            &frame->fix_type,
            &frame->sats[0],
            &frame->sats[1],
            &frame->sats[2],
            &frame->sats[3],
            &frame->sats[4],
            &frame->sats[5],
            &frame->sats[6],
            &frame->sats[7],
            &frame->sats[8],
            &frame->sats[9],
            &frame->sats[10],
            &frame->sats[11],
            &frame->pdop,
            &frame->hdop,
            &frame->vdop))
        return false;
    if (strcmp(type+2, "GSA"))
        return false;

    return true;
}

bool ManuvrGPS::_parse_gll(struct minmea_sentence_gll *frame, const char *sentence) {
    // $GPGLL,3723.2475,N,12158.3416,W,161229.487,A,A*41$;
    char type[6];
    int latitude_direction;
    int longitude_direction;

    if (!_scan(sentence, "tfdfdTc;c",
            type,
            &frame->latitude, &latitude_direction,
            &frame->longitude, &longitude_direction,
            &frame->time,
            &frame->status,
            &frame->mode))
        return false;
    if (strcmp(type+2, "GLL"))
        return false;

    frame->latitude.value *= latitude_direction;
    frame->longitude.value *= longitude_direction;

    return true;
}

bool ManuvrGPS::_parse_gst(struct minmea_sentence_gst *frame, const char *sentence) {
    // $GPGST,024603.00,3.2,6.6,4.7,47.3,5.8,5.6,22.0*58
    char type[6];

    if (!_scan(sentence, "tTfffffff",
            type,
            &frame->time,
            &frame->rms_deviation,
            &frame->semi_major_deviation,
            &frame->semi_minor_deviation,
            &frame->semi_major_orientation,
            &frame->latitude_error_deviation,
            &frame->longitude_error_deviation,
            &frame->altitude_error_deviation))
        return false;
    if (strcmp(type+2, "GST"))
        return false;

    return true;
}

bool ManuvrGPS::_parse_gsv(struct minmea_sentence_gsv *frame, const char *sentence) {
    // $GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74
    // $GPGSV,3,3,11,22,42,067,42,24,14,311,43,27,05,244,00,,,,*4D
    // $GPGSV,4,2,11,08,51,203,30,09,45,215,28*75
    // $GPGSV,4,4,13,39,31,170,27*40
    // $GPGSV,4,4,13*7B
    char type[6];

    if (!_scan(sentence, "tiii;iiiiiiiiiiiiiiii",
            type,
            &frame->total_msgs,
            &frame->msg_nr,
            &frame->total_sats,
            &frame->sats[0].nr,
            &frame->sats[0].elevation,
            &frame->sats[0].azimuth,
            &frame->sats[0].snr,
            &frame->sats[1].nr,
            &frame->sats[1].elevation,
            &frame->sats[1].azimuth,
            &frame->sats[1].snr,
            &frame->sats[2].nr,
            &frame->sats[2].elevation,
            &frame->sats[2].azimuth,
            &frame->sats[2].snr,
            &frame->sats[3].nr,
            &frame->sats[3].elevation,
            &frame->sats[3].azimuth,
            &frame->sats[3].snr
            )) {
        return false;
    }
    if (strcmp(type+2, "GSV"))
        return false;

    return true;
}

bool ManuvrGPS::_parse_vtg(struct minmea_sentence_vtg *frame, const char *sentence) {
    // $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
    // $GPVTG,156.1,T,140.9,M,0.0,N,0.0,K*41
    // $GPVTG,096.5,T,083.5,M,0.0,N,0.0,K,D*22
    // $GPVTG,188.36,T,,M,0.820,N,1.519,K,A*3F
    char type[6];
    char c_true, c_magnetic, c_knots, c_kph, c_faa_mode;

    if (!_scan(sentence, "tfcfcfcfc;c",
            type,
            &frame->true_track_degrees,
            &c_true,
            &frame->magnetic_track_degrees,
            &c_magnetic,
            &frame->speed_knots,
            &c_knots,
            &frame->speed_kph,
            &c_kph,
            &c_faa_mode))
        return false;
    if (strcmp(type+2, "VTG"))
        return false;
    // check chars
    if (c_true != 'T' ||
        c_magnetic != 'M' ||
        c_knots != 'N' ||
        c_kph != 'K')
        return false;
    frame->faa_mode = (minmea_faa_mode) c_faa_mode;

    return true;
}

int ManuvrGPS::_gettime(struct timespec *ts, const struct minmea_date *date, const struct minmea_time *time_) {
    if (date->year == -1 || time_->hours == -1)
        return -1;

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 2000 + date->year - 1900;
    tm.tm_mon = date->month - 1;
    tm.tm_mday = date->day;
    tm.tm_hour = time_->hours;
    tm.tm_min = time_->minutes;
    tm.tm_sec = time_->seconds;

    //time_t timestamp = timegm(&tm); /* See README.md if your system lacks timegm(). */
    time_t timestamp = mktime(&tm); /* See README.md if your system lacks timegm(). */
    if (timestamp != -1) {
        ts->tv_sec = timestamp;
        ts->tv_nsec = time_->microseconds * 1000;
        return 0;
    } else {
        return -1;
    }
}

#endif  //CONFIG_MANUVR_GPS_PIPE
