#include "cycle_utils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTime>

namespace mcclock::services {

static QJsonObject parseCycleData(const QString& cycleData) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(cycleData.toUtf8(), &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        return doc.object();
    }
    return QJsonObject();
}

bool CycleUtils::occursOnDate(const QDate& date, int cycleMode, const QString& cycleData) {
    if (!date.isValid()) return false;
    QJsonObject cd = parseCycleData(cycleData);

    switch (cycleMode) {
    case 0: { // Once
        QDate d = QDate::fromString(cd.value("date").toString(), "yyyy-MM-dd");
        return d.isValid() && d == date;
    }
    case 1: // Daily
        return true;
    case 2: { // Weekly
        int dow = date.dayOfWeek(); // 1=Mon..7=Sun
        const QJsonArray days = cd.value("weekdays").toArray();
        for (const auto& v : days) {
            if (v.toInt() == dow) return true;
        }
        return false;
    }
    case 3: { // Monthly
        int day = cd.value("day").toInt();
        if (day < 1 || day > date.daysInMonth()) return false;
        return date.day() == day;
    }
    case 4: { // Yearly
        int month = cd.value("month").toInt();
        int day = cd.value("day").toInt();
        if (month < 1 || month > 12) return false;
        QDate probe(date.year(), month, 1);
        if (day < 1 || day > probe.daysInMonth()) return false;
        return date.month() == month && date.day() == day;
    }
    case 5: // Interval: handled separately in nextOccurrence
        return true;
    default:
        return false;
    }
}

bool CycleUtils::inRange(const QDateTime& dt, const QString& rangeStart, const QString& rangeEnd) {
    if (!rangeStart.isEmpty()) {
        QDateTime start = QDateTime::fromString(rangeStart, Qt::ISODate);
        if (!start.isValid()) start = QDateTime::fromString(rangeStart.left(10) + "T00:00:00", Qt::ISODate);
        if (start.isValid() && dt < start) return false;
    }
    if (!rangeEnd.isEmpty()) {
        QDateTime end = QDateTime::fromString(rangeEnd, Qt::ISODate);
        if (!end.isValid()) end = QDateTime::fromString(rangeEnd.left(10) + "T23:59:59", Qt::ISODate);
        if (end.isValid() && dt > end) return false;
    }
    return true;
}

QDateTime CycleUtils::nextOccurrence(const QDateTime& now, int cycleMode,
                                     const QString& cycleData, const QString& time,
                                     const QString& rangeStart, const QString& rangeEnd) {
    QJsonObject cd = parseCycleData(cycleData);
    QTime t = QTime::fromString(time, "HH:mm");
    if (!t.isValid()) return QDateTime();

    // Interval mode: repeat every N seconds from anchor
    if (cycleMode == 5) {
        qint64 intervalSec = intervalSeconds(cycleData);
        if (intervalSec <= 0) return QDateTime();

        QDateTime anchor = QDateTime::fromString(cd.value("anchor").toString(), Qt::ISODateWithMs);
        if (!anchor.isValid()) anchor = QDateTime::fromString(cd.value("anchor").toString(), Qt::ISODate);
        if (!anchor.isValid()) {
            anchor = QDateTime(now.date(), t);
        }

        if (anchor > now) {
            return inRange(anchor, rangeStart, rangeEnd) ? anchor : QDateTime();
        }
        qint64 elapsed = anchor.secsTo(now);
        qint64 periods = elapsed / intervalSec + 1;
        QDateTime next = anchor.addSecs(periods * intervalSec);

        // Check interval restrictions (weekdays, date range)
        if (!intervalDateAllowed(next.date(), cycleData)) {
            // Skip to next allowed day, preserving the interval-aligned time of day
            QTime nextTimeOfDay = next.time();
            for (int offset = 1; offset <= 365; ++offset) {
                QDate skipDate = next.date().addDays(offset);
                if (intervalDateAllowed(skipDate, cycleData)) {
                    next = QDateTime(skipDate, nextTimeOfDay);
                    break;
                }
            }
        }

        if (!inRange(next, rangeStart, rangeEnd)) return QDateTime();
        return next;
    }

    // Date-based modes: scan up to 400 days ahead (covers leap-year yearly cycles)
    for (int offset = 0; offset <= 400; ++offset) {
        QDate d = now.date().addDays(offset);
        if (!occursOnDate(d, cycleMode, cycleData)) continue;
        QDateTime candidate(d, t);
        if (candidate <= now) continue;
        if (!inRange(candidate, rangeStart, rangeEnd)) {
            // If past range end, no future trigger exists
            if (!rangeEnd.isEmpty()) return QDateTime();
            continue;
        }
        return candidate;
    }
    return QDateTime();
}

qint64 CycleUtils::intervalSeconds(const QString& cycleData) {
    QJsonObject cd = parseCycleData(cycleData);
    int h = cd.value("interval_hours").toInt();
    int m = cd.value("interval_minutes").toInt();
    int s = cd.value("interval_seconds").toInt();
    // Fallback to old format
    if (h == 0 && m == 0 && s == 0) {
        m = cd.value("interval_minutes").toInt();
    }
    return qint64(h) * 3600 + qint64(m) * 60 + qint64(s);
}

bool CycleUtils::intervalDateAllowed(const QDate& date, const QString& cycleData) {
    QJsonObject cd = parseCycleData(cycleData);

    // Check weekday restriction
    QJsonArray restrictWeekdays = cd.value("restrict_weekdays").toArray();
    if (!restrictWeekdays.isEmpty()) {
        int dow = date.dayOfWeek(); // 1=Mon..7=Sun
        bool found = false;
        for (const auto& v : restrictWeekdays) {
            if (v.toInt() == dow) { found = true; break; }
        }
        if (!found) return false;
    }

    // Check date range restriction
    QString restrictStart = cd.value("restrict_start").toString();
    QString restrictEnd = cd.value("restrict_end").toString();
    if (!restrictStart.isEmpty()) {
        QDate start = QDate::fromString(restrictStart, "yyyy-MM-dd");
        if (start.isValid() && date < start) return false;
    }
    if (!restrictEnd.isEmpty()) {
        QDate end = QDate::fromString(restrictEnd, "yyyy-MM-dd");
        if (end.isValid() && date > end) return false;
    }

    return true;
}

} // namespace mcclock::services
