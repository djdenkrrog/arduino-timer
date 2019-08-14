#ifndef _DATETIME_H_
#define _DATETIME_H_

#include <Arduino.h>

class DateTime {
public:
    DateTime (const char* date, const char* time);
    uint8_t year() const       { return y; }
    uint8_t month() const       { return m; }
    uint8_t day() const         { return d; }
    uint8_t hour() const        { return hh; }
    uint8_t minute() const      { return mm; }
    uint8_t second() const      { return ss; }
    uint8_t dayOfTheWeek() const;

protected:
    uint8_t y, m, d, hh, mm, ss;
};
#endif // _DATETIME_H_
