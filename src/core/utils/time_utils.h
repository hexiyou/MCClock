#pragma once

#include <QString>
#include <QDateTime>
#include <QDate>
#include <QTime>

namespace mcclock::utils {

class TimeUtils {
public:
    // Get current ISO8601 timestamp
    static QString nowISO8601() {
        return QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    }

    // Format a duration in seconds to human readable string (e.g., "2h 30m 15s")
    static QString formatDuration(int totalSeconds) {
        if (totalSeconds < 0) totalSeconds = 0;
        int days = totalSeconds / 86400;
        int hours = (totalSeconds % 86400) / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;

        QString result;
        if (days > 0) result += QString::number(days) + QStringLiteral("\u5929");  // 天
        if (hours > 0) result += QString::number(hours) + QStringLiteral("\u65f6"); // 时
        if (minutes > 0) result += QString::number(minutes) + QStringLiteral("\u5206"); // 分
        result += QString::number(seconds) + QStringLiteral("\u79d2"); // 秒
        return result;
    }

    // Format remaining time for countdown display (e.g., "01:30:00")
    static QString formatCountdown(int totalSeconds) {
        if (totalSeconds < 0) totalSeconds = 0;
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }

    // Format stopwatch time with centiseconds (e.g., "00:01:30:45")
    static QString formatStopwatch(qint64 milliseconds) {
        if (milliseconds < 0) milliseconds = 0;
        int hours = static_cast<int>(milliseconds / 3600000);
        int minutes = static_cast<int>((milliseconds % 3600000) / 60000);
        int seconds = static_cast<int>((milliseconds % 60000) / 1000);
        int centiseconds = static_cast<int>((milliseconds % 1000) / 10);
        return QString("%1:%2:%3:%4")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(centiseconds, 2, 10, QChar('0'));
    }

    // Calculate days between two dates
    static int daysBetween(const QDate& a, const QDate& b) {
        return static_cast<int>(a.daysTo(b));
    }

    // Parse HH:mm time string
    static QTime parseTime(const QString& timeStr) {
        return QTime::fromString(timeStr, "HH:mm");
    }

    // Format time to HH:mm
    static QString formatTime(const QTime& time) {
        return time.toString("HH:mm");
    }

    // Get weekday name in Chinese
    static QString weekdayName(int dayOfWeek) {
        static const char* names[] = {
            "\u5468\u65e5", "\u5468\u4e00", "\u5468\u4e8c", "\u5468\u4e09",
            "\u5468\u56db", "\u5468\u4e94", "\u5468\u516d"
        };
        // Qt: 1=Monday ... 7=Sunday
        if (dayOfWeek == 7) return QString::fromUtf8(names[0]); // Sunday
        return QString::fromUtf8(names[dayOfWeek]);
    }
};

} // namespace mcclock::utils
