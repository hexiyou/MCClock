#include "settings_manager.h"

#include <QFile>
#include <QJsonArray>
#include <QDebug>

namespace mcclock::dal {

SettingsManager& SettingsManager::instance() {
    static SettingsManager mgr;
    return mgr;
}

bool SettingsManager::load(const QString& filePath) {
    filePath_ = filePath;
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();

        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            root_ = doc.object();
            ensureDefaults();
            return true;
        }
        qWarning() << "Settings parse error:" << err.errorString();
    }

    // Create with defaults
    root_ = QJsonObject();
    ensureDefaults();
    save();
    return true;
}

bool SettingsManager::save() {
    if (filePath_.isEmpty()) return false;

    QFile file(filePath_);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(root_);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }
    qWarning() << "Failed to save settings to:" << filePath_;
    return false;
}

void SettingsManager::ensureDefaults() {
    // General
    if (!root_.contains("general")) {
        QJsonObject g;
        g["auto_start"] = false;
        g["missed_reminder"] = true;
        g["auto_check_update"] = true;
        g["language"] = "zh-CN";
        root_["general"] = g;
    }

    // Reminder
    if (!root_.contains("reminder")) {
        QJsonObject r;
        r["fullscreen_mode"] = true;
        r["fullscreen_time_start"] = "22:00";
        r["fullscreen_time_end"] = "07:00";
        r["position"] = "center";
        r["close_mode"] = "manual";
        r["auto_close_minutes"] = 1;
        r["volume"] = 60;
        root_["reminder"] = r;
    }

    // Hourly chime
    if (!root_.contains("hourly_chime")) {
        QJsonObject c;
        c["mode"] = "text_and_voice";
        c["cycle"] = "hourly";
        QJsonArray hours;
        for (int i = 1; i <= 24; ++i) hours.append(i);
        c["hours"] = hours;
        root_["hourly_chime"] = c;
    }

    // Desktop widgets
    if (!root_.contains("desktop_widgets")) {
        QJsonObject d;
        d["desktop_clock"] = true;
        d["desktop_digital_clock"] = true;
        root_["desktop_widgets"] = d;
    }

    // Time sync
    if (!root_.contains("time_sync")) {
        QJsonObject t;
        t["interval_minutes"] = 30;
        root_["time_sync"] = t;
    }

    // Shortcuts
    if (!root_.contains("shortcuts")) {
        QJsonObject s;
        s["background"] = "Ctrl+F3";
        s["voice_announce"] = "Ctrl+F5";
        root_["shortcuts"] = s;
    }

    // HTTP API
    if (!root_.contains("http_api")) {
        QJsonObject a;
        a["enabled"] = false;
        a["bind_ip"] = "127.0.0.1";
        a["port"] = 8080;
        root_["http_api"] = a;
    }

    // Cloud sync stub
    if (!root_.contains("cloud_sync")) {
        QJsonObject c;
        c["server_url"] = "";
        c["auth_token"] = "";
        c["last_sync_time"] = "";
        c["sync_interval_minutes"] = 0;
        root_["cloud_sync"] = c;
    }

    // Version
    if (!root_.contains("version")) {
        root_["version"] = 1;
    }
}

// ── Helper macros ──
#define GET_BOOL(section, key, def) \
    root_[section].toObject()[key].toBool(def)
#define SET_VAL(section, key, val) \
    QJsonObject obj = root_[section].toObject(); obj[key] = val; root_[section] = obj

#define GET_STR(section, key, def) \
    root_[section].toObject()[key].toString(def)
#define GET_INT(section, key, def) \
    root_[section].toObject()[key].toInt(def)

// ── General ──
bool SettingsManager::autoStart() const { return GET_BOOL("general", "auto_start", false); }
void SettingsManager::setAutoStart(bool v) { SET_VAL("general", "auto_start", v); }

bool SettingsManager::missedReminder() const { return GET_BOOL("general", "missed_reminder", true); }
void SettingsManager::setMissedReminder(bool v) { SET_VAL("general", "missed_reminder", v); }

bool SettingsManager::autoCheckUpdate() const { return GET_BOOL("general", "auto_check_update", true); }
void SettingsManager::setAutoCheckUpdate(bool v) { SET_VAL("general", "auto_check_update", v); }

// ── Reminder ──
bool SettingsManager::fullscreenMode() const { return GET_BOOL("reminder", "fullscreen_mode", true); }
void SettingsManager::setFullscreenMode(bool v) { SET_VAL("reminder", "fullscreen_mode", v); }

QString SettingsManager::fullscreenTimeStart() const { return GET_STR("reminder", "fullscreen_time_start", "22:00"); }
void SettingsManager::setFullscreenTimeStart(const QString& v) { SET_VAL("reminder", "fullscreen_time_start", v); }

QString SettingsManager::fullscreenTimeEnd() const { return GET_STR("reminder", "fullscreen_time_end", "07:00"); }
void SettingsManager::setFullscreenTimeEnd(const QString& v) { SET_VAL("reminder", "fullscreen_time_end", v); }

QString SettingsManager::reminderPosition() const { return GET_STR("reminder", "position", "center"); }
void SettingsManager::setReminderPosition(const QString& v) { SET_VAL("reminder", "position", v); }

QString SettingsManager::closeMode() const { return GET_STR("reminder", "close_mode", "manual"); }
void SettingsManager::setCloseMode(const QString& v) { SET_VAL("reminder", "close_mode", v); }

int SettingsManager::autoCloseMinutes() const { return GET_INT("reminder", "auto_close_minutes", 1); }
void SettingsManager::setAutoCloseMinutes(int v) { SET_VAL("reminder", "auto_close_minutes", v); }

int SettingsManager::alarmVolume() const { return GET_INT("reminder", "volume", 60); }
void SettingsManager::setAlarmVolume(int v) { SET_VAL("reminder", "volume", v); }

// ── Hourly chime ──
QString SettingsManager::chimeMode() const { return GET_STR("hourly_chime", "mode", "text_and_voice"); }
void SettingsManager::setChimeMode(const QString& v) { SET_VAL("hourly_chime", "mode", v); }

QString SettingsManager::chimeCycle() const { return GET_STR("hourly_chime", "cycle", "hourly"); }
void SettingsManager::setChimeCycle(const QString& v) { SET_VAL("hourly_chime", "cycle", v); }

QJsonArray SettingsManager::chimeHours() const {
    return root_["hourly_chime"].toObject()["hours"].toArray();
}
void SettingsManager::setChimeHours(const QJsonArray& v) { SET_VAL("hourly_chime", "hours", v); }

int SettingsManager::chimeMinute() const { return GET_INT("hourly_chime", "minute", 0); }
void SettingsManager::setChimeMinute(int v) { SET_VAL("hourly_chime", "minute", v); }

// ── Desktop widgets ──
bool SettingsManager::desktopClock() const { return GET_BOOL("desktop_widgets", "desktop_clock", true); }
void SettingsManager::setDesktopClock(bool v) { SET_VAL("desktop_widgets", "desktop_clock", v); }

bool SettingsManager::desktopDigitalClock() const { return GET_BOOL("desktop_widgets", "desktop_digital_clock", true); }
void SettingsManager::setDesktopDigitalClock(bool v) { SET_VAL("desktop_widgets", "desktop_digital_clock", v); }

// ── Time sync ──
int SettingsManager::timeSyncInterval() const { return GET_INT("time_sync", "interval_minutes", 30); }
void SettingsManager::setTimeSyncInterval(int v) { SET_VAL("time_sync", "interval_minutes", v); }

// ── Shortcuts ──
QString SettingsManager::shortcutBackground() const { return GET_STR("shortcuts", "background", "Ctrl+F3"); }
void SettingsManager::setShortcutBackground(const QString& v) { SET_VAL("shortcuts", "background", v); }

QString SettingsManager::shortcutVoiceAnnounce() const { return GET_STR("shortcuts", "voice_announce", "Ctrl+F5"); }
void SettingsManager::setShortcutVoiceAnnounce(const QString& v) { SET_VAL("shortcuts", "voice_announce", v); }

// ── HTTP API ──
bool SettingsManager::httpApiEnabled() const { return GET_BOOL("http_api", "enabled", false); }
void SettingsManager::setHttpApiEnabled(bool v) { SET_VAL("http_api", "enabled", v); }

QString SettingsManager::httpApiBindIp() const { return GET_STR("http_api", "bind_ip", "127.0.0.1"); }
void SettingsManager::setHttpApiBindIp(const QString& v) { SET_VAL("http_api", "bind_ip", v); }

int SettingsManager::httpApiPort() const { return GET_INT("http_api", "port", 8080); }
void SettingsManager::setHttpApiPort(int v) { SET_VAL("http_api", "port", v); }

// ── Cloud sync ──
QString SettingsManager::cloudServerUrl() const { return GET_STR("cloud_sync", "server_url", ""); }
void SettingsManager::setCloudServerUrl(const QString& v) { SET_VAL("cloud_sync", "server_url", v); }

QString SettingsManager::cloudAuthToken() const { return GET_STR("cloud_sync", "auth_token", ""); }
void SettingsManager::setCloudAuthToken(const QString& v) { SET_VAL("cloud_sync", "auth_token", v); }

QString SettingsManager::lastSyncTime() const { return GET_STR("cloud_sync", "last_sync_time", ""); }
void SettingsManager::setLastSyncTime(const QString& v) { SET_VAL("cloud_sync", "last_sync_time", v); }

// ── Generic ──
QJsonValue SettingsManager::get(const QStringList& path) const {
    QJsonValue current(root_);
    for (const auto& key : path) {
        if (!current.isObject()) return QJsonValue();
        current = current.toObject()[key];
    }
    return current;
}

void SettingsManager::set(const QStringList& path, const QJsonValue& value) {
    if (path.isEmpty()) return;
    if (path.size() == 1) {
        root_[path[0]] = value;
        return;
    }
    // Navigate to parent
    QJsonObject* current = &root_;
    QVector<QJsonObject*> stack;
    stack.append(current);
    for (int i = 0; i < path.size() - 1; ++i) {
        if (!current->contains(path[i]) || !(*current)[path[i]].isObject()) {
            (*current)[path[i]] = QJsonObject();
        }
        QJsonObject child = (*current)[path[i]].toObject();
        (*current)[path[i]] = child;
        // For nested setting, we need a different approach
        // This simplified version handles 2-level paths
    }
    // Simplified: handle 2-level paths
    if (path.size() == 2) {
        QJsonObject section = root_[path[0]].toObject();
        section[path[1]] = value;
        root_[path[0]] = section;
    }
}

QJsonObject SettingsManager::rootObject() const {
    return root_;
}

#undef GET_BOOL
#undef SET_VAL
#undef GET_STR
#undef GET_INT

} // namespace mcclock::dal
