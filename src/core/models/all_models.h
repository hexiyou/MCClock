#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace mcclock::models {

// ── Sync status for cloud sync preparation ──
enum class SyncStatus : int {
    New      = 0,
    Synced   = 1,
    Modified = 2,
    Deleted  = 3
};

// ── Cycle mode for recurring tasks ──
enum class CycleMode : int {
    Once    = 0,
    Daily   = 1,
    Weekly  = 2,
    Monthly = 3,
    Yearly  = 4,
    Interval = 5
};

// ── Ring mode for alarm sounds ──
enum class RingMode : int {
    AnnounceTime = 0,  // Ring and announce time
    Continuous   = 1,  // Ring continuously
    Once         = 2,  // Ring once
    Silent       = 3,  // Silent
    Custom       = 4   // Custom duration (minutes)
};

// ── Shutdown option types ──
enum class ShutdownOption : int {
    ForceShutdown = 0,
    NormalShutdown = 1,
    Restart       = 2,
    Logoff        = 3
};

// ── Countdown mode ──
enum class CountdownMode : int {
    Relative = 0,  // Count down from a duration
    Absolute = 1   // Count down to a specific datetime
};

// ── Health reminder display mode ──
enum class HealthDisplayMode : int {
    Fullscreen = 0,
    Window     = 1
};

// ── Alarm Group ──
struct AlarmGroup {
    QString uuid;
    int syncStatus = static_cast<int>(SyncStatus::New);
    QString lastModified;
    QString createdAt;

    QString name;
    int sortOrder = 0;

    QJsonObject toJson() const;
    static AlarmGroup fromJson(const QJsonObject& json);
};

// ── Alarm ──
struct Alarm {
    QString uuid;
    int syncStatus = static_cast<int>(SyncStatus::New);
    QString lastModified;
    QString createdAt;

    bool enabled = true;
    int cycleMode = static_cast<int>(CycleMode::Once);
    QString cycleData;          // JSON: weekday numbers / day of month / month-day / interval params
    QString time;               // HH:mm format
    QString rangeStart;         // Optional start date (ISO8601)
    QString rangeEnd;           // Optional end date (ISO8601)
    int ringtone = 1;           // 1-6 builtin, 7 random, 8 custom
    QString customRingtonePath;
    int ringMode = static_cast<int>(RingMode::AnnounceTime);
    int customMinutes = 0;
    QString label;
    QString groupId;            // FK -> alarm_groups.uuid
    bool deleted = false;       // Recycle bin flag

    QJsonObject toJson() const;
    static Alarm fromJson(const QJsonObject& json);
};

// ── Birthday ──
struct Birthday {
    QString uuid;
    int syncStatus = static_cast<int>(SyncStatus::New);
    QString lastModified;
    QString createdAt;

    QString name;
    int gender = 0;             // 0 = male, 1 = female
    bool isLunar = false;       // false = Gregorian, true = Lunar
    int solarYear = 0;
    int solarMonth = 0;
    int solarDay = 0;
    int lunarMonth = 0;
    int lunarDay = 0;
    QString remindTime;         // HH:mm format, default "08:00"
    int advanceDays = 1;        // 1-5 days before birthday
    int ringtone = 1;
    QString customRingtonePath;
    int ringMode = static_cast<int>(RingMode::AnnounceTime);
    int customMinutes = 0;
    QString avatarPath;         // Path to 50x50 avatar image
    QString label;

    QJsonObject toJson() const;
    static Birthday fromJson(const QJsonObject& json);
};

// ── Shutdown Task ──
struct ShutdownTask {
    QString uuid;
    int syncStatus = static_cast<int>(SyncStatus::New);
    QString lastModified;
    QString createdAt;

    bool enabled = true;
    int cycleMode = static_cast<int>(CycleMode::Once);
    QString cycleData;
    QString time;
    QString rangeStart;
    QString rangeEnd;
    int shutdownOption = static_cast<int>(ShutdownOption::NormalShutdown);
    int advanceSeconds = 30;    // Seconds to warn before shutdown
    QString label;

    QJsonObject toJson() const;
    static ShutdownTask fromJson(const QJsonObject& json);
};

// ── Run Program Task ──
struct RunProgramTask {
    QString uuid;
    int syncStatus = static_cast<int>(SyncStatus::New);
    QString lastModified;
    QString createdAt;

    bool enabled = true;
    int cycleMode = static_cast<int>(CycleMode::Once);
    QString cycleData;
    QString time;
    QString rangeStart;
    QString rangeEnd;
    QString programPath;        // Path to executable or URL
    QString arguments;          // Command line arguments
    bool ringEnabled = false;
    QString label;

    QJsonObject toJson() const;
    static RunProgramTask fromJson(const QJsonObject& json);
};

// ── Countdown ──
struct Countdown {
    QString uuid;
    int syncStatus = static_cast<int>(SyncStatus::New);
    QString lastModified;
    QString createdAt;

    bool enabled = true;
    int mode = static_cast<int>(CountdownMode::Relative);
    int totalSeconds = 0;       // For relative mode
    QString targetDatetime;     // For absolute mode (ISO8601)
    int remainingSeconds = 0;   // Current remaining (for relative mode)
    int ringtone = 1;
    QString customRingtonePath;
    int ringMode = static_cast<int>(RingMode::Continuous);
    int customMinutes = 0;
    QString label;

    QJsonObject toJson() const;
    static Countdown fromJson(const QJsonObject& json);
};

// ── Health Settings ──
struct HealthSettings {
    QString uuid;
    int syncStatus = static_cast<int>(SyncStatus::New);
    QString lastModified;
    QString createdAt;

    bool enabled = false;
    int displayMode = static_cast<int>(HealthDisplayMode::Window);
    int workMinutes = 45;       // 1-999
    int restMinutes = 5;        // 1-99
    int ringtone = 1;
    QString customRingtonePath;
    int ringMode = static_cast<int>(RingMode::AnnounceTime);
    int customMinutes = 0;
    QString label;

    QJsonObject toJson() const;
    static HealthSettings fromJson(const QJsonObject& json);
};

} // namespace mcclock::models
