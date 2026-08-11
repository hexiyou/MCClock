#include "lunar_calendar.h"

namespace mcclock::services {

// Lunar calendar data table 1900-2100.
// Each entry encodes: leap month (bits 0-3), month sizes for 12 months (bits 4-15),
// leap month size (bit 16).
const unsigned int LunarCalendar::lunarInfo[] = {
    0x04bd8, 0x04ae0, 0x0a570, 0x054d5, 0x0d260, 0x0d950, 0x16554, 0x056a0, 0x09ad0, 0x055d2, // 1900-1909
    0x04ae0, 0x0a5b6, 0x0a4d0, 0x0d250, 0x1d255, 0x0b540, 0x0d6a0, 0x0ada2, 0x095b0, 0x14977, // 1910-1919
    0x04970, 0x0a4b0, 0x0b4b5, 0x06a50, 0x06d40, 0x1ab54, 0x02b60, 0x09570, 0x052f2, 0x04970, // 1920-1929
    0x06566, 0x0d4a0, 0x0ea50, 0x06e95, 0x05ad0, 0x02b60, 0x186e3, 0x092e0, 0x1c8d7, 0x0c950, // 1930-1939
    0x0d4a0, 0x1d8a6, 0x0b550, 0x056a0, 0x1a5b4, 0x025d0, 0x092d0, 0x0d2b2, 0x0a950, 0x0b557, // 1940-1949
    0x06ca0, 0x0b550, 0x15355, 0x04da0, 0x0a5b0, 0x14573, 0x052b0, 0x0a9a8, 0x0e950, 0x06aa0, // 1950-1959
    0x0aea6, 0x0ab50, 0x04b60, 0x0aae4, 0x0a570, 0x05260, 0x0f263, 0x0d950, 0x05b57, 0x056a0, // 1960-1969
    0x096d0, 0x04dd5, 0x04ad0, 0x0a4d0, 0x0d4d4, 0x0d250, 0x0d558, 0x0b540, 0x0b6a0, 0x195a6, // 1970-1979
    0x095b0, 0x049b0, 0x0a974, 0x0a4b0, 0x0b27a, 0x06a50, 0x06d40, 0x0af46, 0x0ab60, 0x09570, // 1980-1989
    0x04af5, 0x04970, 0x064b0, 0x074a3, 0x0ea50, 0x06b58, 0x055c0, 0x0ab60, 0x096d5, 0x092e0, // 1990-1999
    0x0c960, 0x0d954, 0x0d4a0, 0x0da50, 0x07552, 0x056a0, 0x0abb7, 0x025d0, 0x092d0, 0x0cab5, // 2000-2009
    0x0a950, 0x0b4a0, 0x0baa4, 0x0ad50, 0x055d9, 0x04ba0, 0x0a5b0, 0x15176, 0x052b0, 0x0a930, // 2010-2019
    0x07954, 0x06aa0, 0x0ad50, 0x05b52, 0x04b60, 0x0a6e6, 0x0a4e0, 0x0d260, 0x0ea65, 0x0d530, // 2020-2029
    0x05aa0, 0x076a3, 0x096d0, 0x04afb, 0x04ad0, 0x0a4d0, 0x1d0b6, 0x0d250, 0x0d520, 0x0dd45, // 2030-2039
    0x0b5a0, 0x056d0, 0x055b2, 0x049b0, 0x0a577, 0x0a4b0, 0x0aa50, 0x1b255, 0x06d20, 0x0ada0, // 2040-2049
    0x14b63, 0x09370, 0x049f8, 0x04970, 0x064b0, 0x168a6, 0x0ea50, 0x06b20, 0x1a6c4, 0x0aae0, // 2050-2059
    0x0a2e0, 0x0d2e3, 0x0c960, 0x0d557, 0x0d4a0, 0x0da50, 0x05d55, 0x056a0, 0x0a6d0, 0x055d4, // 2060-2069
    0x052d0, 0x0a9b8, 0x0a950, 0x0b4a0, 0x0b6a6, 0x0ad50, 0x055a0, 0x0aba4, 0x0a5b0, 0x052b0, // 2070-2079
    0x0b273, 0x06930, 0x07337, 0x06aa0, 0x0ad50, 0x14b55, 0x04b60, 0x0a570, 0x054e4, 0x0d160, // 2080-2089
    0x0e968, 0x0d520, 0x0daa0, 0x16aa6, 0x056d0, 0x04ae0, 0x0a9d4, 0x0a2d0, 0x0d150, 0x0f252, // 2090-2099
    0x0d520 // 2100
};

const int LunarCalendar::LUNAR_START_YEAR = 1900;

int LunarCalendar::yearDays(int y) {
    int i, sum = 348;
    for (i = 0x8000; i > 0x8; i >>= 1) {
        sum += (lunarInfo[y - LUNAR_START_YEAR] & i) ? 1 : 0;
    }
    return sum + leapDays(y);
}

int LunarCalendar::leapDays(int y) {
    if (leapMonth(y)) {
        return (lunarInfo[y - LUNAR_START_YEAR] & 0x10000) ? 30 : 29;
    }
    return 0;
}

int LunarCalendar::leapMonth(int y) {
    return lunarInfo[y - LUNAR_START_YEAR] & 0xf;
}

int LunarCalendar::monthDays(int y, int m) {
    return (lunarInfo[y - LUNAR_START_YEAR] & (0x10000 >> m)) ? 30 : 29;
}

int LunarCalendar::getLunarMonthDays(int year, int month) {
    return monthDays(year, month);
}

int LunarCalendar::getLeapMonthDays(int year) {
    return leapDays(year);
}

int LunarCalendar::getLeapMonth(int year) {
    return leapMonth(year);
}

int LunarCalendar::getSolarYearDays(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 366 : 365;
}

LunarDate LunarCalendar::solarToLunar(int year, int month, int day) {
    LunarDate result;
    // Days offset from 1900-01-31 (lunar 1900-01-01)
    int offset = 0;
    // Calculate days between 1900-01-31 and the given date
    // Use a simple day count
    int y, m, d;

    // Count days from 1900-01-31 to year-month-day
    offset = 0;
    for (y = 1900; y < year; ++y) {
        offset += getSolarYearDays(y);
    }
    // Days in current year up to month-day
    static const int monthDaysArr[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    for (m = 1; m < month; ++m) {
        offset += monthDaysArr[m-1];
        if (m == 2 && getSolarYearDays(year) == 366) offset += 1;
    }
    offset += day;
    // 1900-01-31 is day 31 of year 1900
    offset -= 31;

    // Determine lunar year
    int lunarYear = 1900;
    int temp = 0;
    for (y = 1900; y < 2101 && offset > 0; ++y) {
        temp = yearDays(y);
        offset -= temp;
        lunarYear++;
    }
    if (offset < 0) {
        offset += temp;
        lunarYear--;
    }

    result.year = lunarYear;

    // Determine lunar month
    int leap = leapMonth(lunarYear);
    bool isLeap = false;
    int lunarMonth = 1;
    for (m = 1; m <= 12 && offset > 0; ++m) {
        // Normal month
        temp = monthDays(lunarYear, m);
        if (offset >= temp) {
            offset -= temp;
            lunarMonth++;
        } else {
            break;
        }
        // Leap month follows the same-numbered month
        if (m == leap && offset > 0) {
            temp = leapDays(lunarYear);
            if (offset >= temp) {
                offset -= temp;
                // still same month number but leap
            } else {
                isLeap = true;
                break;
            }
        }
    }
    if (offset == 0 && leap == lunarMonth && !isLeap) {
        // Edge: exactly at leap month start
    }

    result.month = lunarMonth;
    result.isLeapMonth = isLeap;
    result.day = offset + 1;

    return result;
}

void LunarCalendar::lunarToSolar(int lunarYear, int lunarMonth, int lunarDay,
                                 bool isLeap, int& solarYear, int& solarMonth, int& solarDay) {
    // Count days from lunar 1900-01-01
    int offset = 0;
    for (int y = 1900; y < lunarYear; ++y) {
        offset += yearDays(y);
    }
    for (int m = 1; m < lunarMonth; ++m) {
        offset += monthDays(lunarYear, m);
        if (m == leapMonth(lunarYear)) offset += leapDays(lunarYear);
    }
    if (isLeap) {
        offset += monthDays(lunarYear, lunarMonth);
    }
    offset += lunarDay - 1;

    // Convert offset days from 1900-01-31 to solar date
    int days = offset + 31; // day-of-year offset in 1900 (Jan 31 = day 31)
    int y = 1900;
    while (days > getSolarYearDays(y)) {
        days -= getSolarYearDays(y);
        y++;
    }
    solarYear = y;

    static const int monthDaysArr[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int m = 0;
    int mdays = 0;
    for (m = 0; m < 12; ++m) {
        mdays = monthDaysArr[m];
        if (m == 1 && getSolarYearDays(y) == 366) mdays = 29;
        if (days <= mdays) break;
        days -= mdays;
    }
    solarMonth = m + 1;
    solarDay = days;
}

QString LunarCalendar::getZodiacAnimal(int year) {
    static const char* animals[] = {
        "\u9f20", "\u725b", "\u864e", "\u5154", "\u9f99", "\u86c7",
        "\u9a6c", "\u7f8a", "\u7334", "\u9e21", "\u72d7", "\u732a"
    };
    // 1900 is 鼠 (rat)
    int idx = (year - 1900) % 12;
    if (idx < 0) idx += 12;
    return QString::fromUtf8(animals[idx]);
}

QString LunarCalendar::getConstellation(int month, int day) {
    static const char* names[] = {
        "\u6c34\u74f6\u5ea7", "\u53cc\u9c7c\u5ea7", "\u767d\u7f8a\u5ea7", "\u91d1\u725b\u5ea7",
        "\u53cc\u5b50\u5ea7", "\u5de8\u87f9\u5ea7", "\u72ee\u5b50\u5ea7", "\u5904\u5973\u5ea7",
        "\u5929\u79e4\u5ea7", "\u5929\u874e\u5ea7", "\u5c04\u624b\u5ea7", "\u6469\u7faf\u5ea7"
    };
    static const int edge[] = {20, 19, 21, 20, 21, 22, 23, 23, 23, 24, 23, 22};
    int idx = month - 1;
    if (day < edge[idx]) {
        idx = (idx + 11) % 12;
    }
    return QString::fromUtf8(names[idx]);
}

QString LunarCalendar::getLunarMonthName(int month, bool isLeap) {
    static const char* names[] = {
        "\u6b63\u6708", "\u4e8c\u6708", "\u4e09\u6708", "\u56db\u6708",
        "\u4e94\u6708", "\u516d\u6708", "\u4e03\u6708", "\u516b\u6708",
        "\u4e5d\u6708", "\u5341\u6708", "\u51ac\u6708", "\u814a\u6708"
    };
    QString prefix = isLeap ? QString::fromUtf8("\u95f0") : QString();
    if (month >= 1 && month <= 12) {
        return prefix + QString::fromUtf8(names[month - 1]);
    }
    return prefix + QString::number(month);
}

QString LunarCalendar::getLunarDayName(int day) {
    static const char* names[] = {
        "\u521d\u4e00", "\u521d\u4e8c", "\u521d\u4e09", "\u521d\u56db", "\u521d\u4e94",
        "\u521d\u516d", "\u521d\u4e03", "\u521d\u516b", "\u521d\u4e5d", "\u521d\u5341",
        "\u5341\u4e00", "\u5341\u4e8c", "\u5341\u4e09", "\u5341\u56db", "\u5341\u4e94",
        "\u5341\u516d", "\u5341\u4e03", "\u5341\u516b", "\u5341\u4e5d", "\u4e8c\u5341",
        "\u5eff\u4e00", "\u5eff\u4e8c", "\u5eff\u4e09", "\u5eff\u4e8c", "\u5eff\u4e94",
        "\u5eff\u516d", "\u5eff\u4e03", "\u5eff\u516b", "\u5eff\u4e5d", "\u4e09\u5341"
    };
    if (day >= 1 && day <= 30) {
        return QString::fromUtf8(names[day - 1]);
    }
    return QString::number(day);
}

} // namespace mcclock::services
