#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QStringList>

namespace mcclock::dal {

class SettingsManager {
public:
    static SettingsManager& instance();

    // Load settings from JSON file
    bool load(const QString& filePath);

    // Save settings to JSON file
    bool save();

    // ── General settings ──
    bool autoStart() const;
    void setAutoStart(bool v);

    bool missedReminder() const;
    void setMissedReminder(bool v);

    bool autoCheckUpdate() const;
    void setAutoCheckUpdate(bool v);

    // ── Reminder settings ──
    bool fullscreenMode() const;
    void setFullscreenMode(bool v);

    QString fullscreenTimeStart() const;
    void setFullscreenTimeStart(const QString& v);

    QString fullscreenTimeEnd() const;
    void setFullscreenTimeEnd(const QString& v);

    QString reminderPosition() const;  // "center", "left_up", "left_down"
    void setReminderPosition(const QString& v);

    QString closeMode() const;  // "manual", "auto"
    void setCloseMode(const QString& v);

    int autoCloseMinutes() const;
    void setAutoCloseMinutes(int v);

    int alarmVolume() const;  // 0-100
    void setAlarmVolume(int v);

    // ── Hourly chime ──
    QString chimeMode() const;  // "text_and_voice", "text", "voice", "off"
    void setChimeMode(const QString& v);

    QString chimeCycle() const;  // "hourly", "half_hour", "custom"
    void setChimeCycle(const QString& v);

    QJsonArray chimeHours() const;  // Array of ints 1-24
    void setChimeHours(const QJsonArray& v);

    int chimeMinute() const;  // Custom chime minute (0-59), default 0
    void setChimeMinute(int v);

    // ── Desktop widgets ──
    bool desktopClock() const;
    void setDesktopClock(bool v);

    bool desktopDigitalClock() const;
    void setDesktopDigitalClock(bool v);

    // ── Time sync ──
    int timeSyncInterval() const;  // minutes
    void setTimeSyncInterval(int v);

    // ── Shortcuts ──
    QString shortcutBackground() const;
    void setShortcutBackground(const QString& v);

    QString shortcutVoiceAnnounce() const;
    void setShortcutVoiceAnnounce(const QString& v);

    // ── HTTP API ──
    bool httpApiEnabled() const;
    void setHttpApiEnabled(bool v);

    QString httpApiBindIp() const;
    void setHttpApiBindIp(const QString& v);

    int httpApiPort() const;
    void setHttpApiPort(int v);

    // ── Cloud sync (stub) ──
    QString cloudServerUrl() const;
    void setCloudServerUrl(const QString& v);

    QString cloudAuthToken() const;
    void setCloudAuthToken(const QString& v);

    QString lastSyncTime() const;
    void setLastSyncTime(const QString& v);

    // ── Generic accessors ──
    QJsonValue get(const QStringList& path) const;
    void set(const QStringList& path, const QJsonValue& value);

    // Get the full JSON root (for API responses)
    QJsonObject rootObject() const;

private:
    SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QJsonObject root_;
    QString filePath_;

    void ensureDefaults();
};

} // namespace mcclock::dal
