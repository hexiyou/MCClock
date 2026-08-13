#pragma once

#include <QString>
#include <QDate>
#include <QDateTime>

namespace mcclock::services {

// Computes trigger times for the 6 cycle modes.
// cycleData JSON formats:
//   Once:     {"date":"yyyy-MM-dd"}
//   Daily:    {}
//   Weekly:   {"weekdays":[1..7]}  (1=Monday .. 7=Sunday)
//   Monthly:  {"day":N}            (skips months without day N)
//   Yearly:   {"month":M,"day":D}  (skips invalid dates like Feb 30)
//   Interval: {"interval_hours":H,"interval_minutes":M,"interval_seconds":S,
//              "anchor":"ISO8601",
//              "restrict_weekdays":[1..7],           (optional, empty=all days)
//              "restrict_start":"yyyy-MM-dd",        (optional)
//              "restrict_end":"yyyy-MM-dd"}          (optional)
class CycleUtils {
public:
    // Whether the cycle triggers on the given date
    static bool occursOnDate(const QDate& date, int cycleMode, const QString& cycleData);

    // Next trigger datetime strictly after `now`; invalid QDateTime if none
    // (e.g. once-task already passed, or out of range_start/range_end)
    static QDateTime nextOccurrence(const QDateTime& now, int cycleMode,
                                    const QString& cycleData, const QString& time,
                                    const QString& rangeStart, const QString& rangeEnd);

    // Whether a trigger at the given datetime falls within the optional range
    static bool inRange(const QDateTime& dt, const QString& rangeStart, const QString& rangeEnd);

    // Get interval total seconds from cycleData (supports hours, minutes, seconds)
    static qint64 intervalSeconds(const QString& cycleData);

    // Check if date is within interval restrictions (weekdays, date range)
    static bool intervalDateAllowed(const QDate& date, const QString& cycleData);
};

} // namespace mcclock::services
