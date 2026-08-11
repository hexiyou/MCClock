#pragma once

#include <QString>

namespace mcclock::services {

// Lunar (Chinese traditional) calendar date
struct LunarDate {
    int year = 0;
    int month = 0;        // 1-12, leap month indicated by isLeapMonth
    int day = 0;          // 1-30
    bool isLeapMonth = false;
};

// Chinese lunar calendar conversion using lookup table (1900-2100)
class LunarCalendar {
public:
    // Convert Gregorian (solar) date to lunar date
    static LunarDate solarToLunar(int year, int month, int day);

    // Convert lunar date to Gregorian (solar) date
    static void lunarToSolar(int lunarYear, int lunarMonth, int lunarDay,
                             bool isLeap, int& solarYear, int& solarMonth, int& solarDay);

    // Get Chinese zodiac animal for a year (e.g., 鼠,牛,虎...)
    static QString getZodiacAnimal(int year);

    // Get Western constellation for a solar month/day (e.g., 白羊座...)
    static QString getConstellation(int month, int day);

    // Get lunar month name (正月,二月,...,冬月,腊月)
    static QString getLunarMonthName(int month, bool isLeap = false);

    // Get lunar day name (初一,初二,...,三十)
    static QString getLunarDayName(int day);

    // Get number of days in a lunar month for a given year
    static int getLunarMonthDays(int year, int month);

    // Get number of days in a leap month (0 if no leap month that year)
    static int getLeapMonthDays(int year);

    // Get which month is leap in a year (0 if none)
    static int getLeapMonth(int year);

    // Get days in a solar year
    static int getSolarYearDays(int year);

private:
    // Lunar data table 1900-2100
    static const unsigned int lunarInfo[];
    static const int LUNAR_START_YEAR;  // 1900

    static int yearDays(int y);
    static int leapDays(int y);
    static int leapMonth(int y);
    static int monthDays(int y, int m);
};

} // namespace mcclock::services
