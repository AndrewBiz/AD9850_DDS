// Logging library forked from https://github.com/NOX73/Arduino-logging-library.git
// Modified by AndrewBiz

#include "Logging.h"

void Logging::Init(int level, long baud, bool print_ts, bool auto_ln){
  _level = constrain(level,LOG_LEVEL_NOOUTPUT,LOG_LEVEL_VERBOSE);
  _baud = baud;
  _print_ts = print_ts;
  _auto_ln = auto_ln;
  Serial.begin(_baud);
}

void Logging::print(const __FlashStringHelper *format, va_list args) {
  PGM_P p = reinterpret_cast<PGM_P>(format);
  char c = pgm_read_byte(p++);
  for(;c != 0; c = pgm_read_byte(p++)){
    if (c == '%') {
      c = pgm_read_byte(p++);
      printFormat(c, &args);
    } else {
      Serial.print(c);
    }
  }

}

void Logging::print(const char *format, va_list args) {
  for (; *format != 0; ++format) {
    if (*format == '%') {
      ++format;
      printFormat(*format, &args);
    } else {
      Serial.print(*format);
    }
  }
}

void Logging::printFormat(const char format, va_list *args) {
  if (format == '\0') return;

  if (format == '%') {
    Serial.print(format);
    return;
  }

  if( format == 's' ) {
    register char *s = (char *)va_arg( *args, int );
    Serial.print(s);
    return;
  }

  if( format == 'd' || format == 'i') {
    Serial.print(va_arg( *args, int ),DEC);
    return;
  }

  if( format == 'x' ) {
    Serial.print(va_arg( *args, int ),HEX);
    return;
  }

  if( format == 'X' ) {
    Serial.print("0x");
    Serial.print(va_arg( *args, int ),HEX);
    return;
  }

  if( format == 'b' ) {
    Serial.print(va_arg( *args, int ),BIN);
    return;
  }

  if( format == 'B' ) {
    Serial.print("0b");
    Serial.print(va_arg( *args, int ),BIN);
    return;
  }

  if( format == 'l' ) {
    Serial.print(va_arg( *args, long ),DEC);
    return;
  }

  if( format == 'c' ) {
    Serial.print(va_arg( *args, int ));
    return;
  }

  if( format == 't' ) {
    if (va_arg( *args, int ) == 1) {
      Serial.print("T");
    }
    else {
      Serial.print("F");
    }
    return;
  }

  if( format == 'T' ) {
    if (va_arg( *args, int ) == 1) {
      Serial.print(F("true"));
    }
    else {
      Serial.print(F("false"));
    }
    return;
  }
}

// Print of the Time Stamp
void Logging::print_ts() {
  if( _print_ts ) {
    char tsbuf[16];
    sprintf( tsbuf, "%010lu ms: ", millis() );
    Serial.print(tsbuf);
  }
}

 Logging Log = Logging();

