#include "all_models.h"

namespace mcclock::models {

// ── Helper macros for JSON serialization ──
#define TO_JSON_STRING(key, val) json[#key] = val
#define TO_JSON_INT(key, val) json[#key] = val
#define TO_JSON_BOOL(key, val) json[#key] = val

#define FROM_JSON_STRING(key, field) if (json.contains(#key)) field = json[#key].toString()
#define FROM_JSON_INT(key, field) if (json.contains(#key)) field = json[#key].toInt()
#define FROM_JSON_BOOL(key, field) if (json.contains(#key)) field = json[#key].toBool()

// ── AlarmGroup ──
QJsonObject AlarmGroup::toJson() const {
    QJsonObject json;
    TO_JSON_STRING(uuid, uuid);
    TO_JSON_INT(syncStatus, syncStatus);
    TO_JSON_STRING(lastModified, lastModified);
    TO_JSON_STRING(createdAt, createdAt);
    TO_JSON_STRING(name, name);
    TO_JSON_INT(sortOrder, sortOrder);
    return json;
}

AlarmGroup AlarmGroup::fromJson(const QJsonObject& json) {
    AlarmGroup g;
    FROM_JSON_STRING(uuid, g.uuid);
    FROM_JSON_INT(syncStatus, g.syncStatus);
    FROM_JSON_STRING(lastModified, g.lastModified);
    FROM_JSON_STRING(createdAt, g.createdAt);
    FROM_JSON_STRING(name, g.name);
    FROM_JSON_INT(sortOrder, g.sortOrder);
    return g;
}

// ── Alarm ──
QJsonObject Alarm::toJson() const {
    QJsonObject json;
    TO_JSON_STRING(uuid, uuid);
    TO_JSON_INT(syncStatus, syncStatus);
    TO_JSON_STRING(lastModified, lastModified);
    TO_JSON_STRING(createdAt, createdAt);
    TO_JSON_BOOL(enabled, enabled);
    TO_JSON_INT(cycleMode, cycleMode);
    TO_JSON_STRING(cycleData, cycleData);
    TO_JSON_STRING(time, time);
    TO_JSON_STRING(rangeStart, rangeStart);
    TO_JSON_STRING(rangeEnd, rangeEnd);
    TO_JSON_INT(ringtone, ringtone);
    TO_JSON_STRING(customRingtonePath, customRingtonePath);
    TO_JSON_INT(ringMode, ringMode);
    TO_JSON_INT(customMinutes, customMinutes);
    TO_JSON_STRING(label, label);
    TO_JSON_STRING(groupId, groupId);
    TO_JSON_BOOL(deleted, deleted);
    return json;
}

Alarm Alarm::fromJson(const QJsonObject& json) {
    Alarm a;
    FROM_JSON_STRING(uuid, a.uuid);
    FROM_JSON_INT(syncStatus, a.syncStatus);
    FROM_JSON_STRING(lastModified, a.lastModified);
    FROM_JSON_STRING(createdAt, a.createdAt);
    FROM_JSON_BOOL(enabled, a.enabled);
    FROM_JSON_INT(cycleMode, a.cycleMode);
    FROM_JSON_STRING(cycleData, a.cycleData);
    FROM_JSON_STRING(time, a.time);
    FROM_JSON_STRING(rangeStart, a.rangeStart);
    FROM_JSON_STRING(rangeEnd, a.rangeEnd);
    FROM_JSON_INT(ringtone, a.ringtone);
    FROM_JSON_STRING(customRingtonePath, a.customRingtonePath);
    FROM_JSON_INT(ringMode, a.ringMode);
    FROM_JSON_INT(customMinutes, a.customMinutes);
    FROM_JSON_STRING(label, a.label);
    FROM_JSON_STRING(groupId, a.groupId);
    FROM_JSON_BOOL(deleted, a.deleted);
    return a;
}

// ── Birthday ──
QJsonObject Birthday::toJson() const {
    QJsonObject json;
    TO_JSON_STRING(uuid, uuid);
    TO_JSON_INT(syncStatus, syncStatus);
    TO_JSON_STRING(lastModified, lastModified);
    TO_JSON_STRING(createdAt, createdAt);
    TO_JSON_STRING(name, name);
    TO_JSON_INT(gender, gender);
    TO_JSON_BOOL(isLunar, isLunar);
    TO_JSON_INT(solarYear, solarYear);
    TO_JSON_INT(solarMonth, solarMonth);
    TO_JSON_INT(solarDay, solarDay);
    TO_JSON_INT(lunarMonth, lunarMonth);
    TO_JSON_INT(lunarDay, lunarDay);
    TO_JSON_STRING(remindTime, remindTime);
    TO_JSON_INT(advanceDays, advanceDays);
    TO_JSON_INT(ringtone, ringtone);
    TO_JSON_STRING(customRingtonePath, customRingtonePath);
    TO_JSON_INT(ringMode, ringMode);
    TO_JSON_INT(customMinutes, customMinutes);
    TO_JSON_STRING(avatarPath, avatarPath);
    TO_JSON_STRING(label, label);
    return json;
}

Birthday Birthday::fromJson(const QJsonObject& json) {
    Birthday b;
    FROM_JSON_STRING(uuid, b.uuid);
    FROM_JSON_INT(syncStatus, b.syncStatus);
    FROM_JSON_STRING(lastModified, b.lastModified);
    FROM_JSON_STRING(createdAt, b.createdAt);
    FROM_JSON_STRING(name, b.name);
    FROM_JSON_INT(gender, b.gender);
    FROM_JSON_BOOL(isLunar, b.isLunar);
    FROM_JSON_INT(solarYear, b.solarYear);
    FROM_JSON_INT(solarMonth, b.solarMonth);
    FROM_JSON_INT(solarDay, b.solarDay);
    FROM_JSON_INT(lunarMonth, b.lunarMonth);
    FROM_JSON_INT(lunarDay, b.lunarDay);
    FROM_JSON_STRING(remindTime, b.remindTime);
    FROM_JSON_INT(advanceDays, b.advanceDays);
    FROM_JSON_INT(ringtone, b.ringtone);
    FROM_JSON_STRING(customRingtonePath, b.customRingtonePath);
    FROM_JSON_INT(ringMode, b.ringMode);
    FROM_JSON_INT(customMinutes, b.customMinutes);
    FROM_JSON_STRING(avatarPath, b.avatarPath);
    FROM_JSON_STRING(label, b.label);
    return b;
}

// ── ShutdownTask ──
QJsonObject ShutdownTask::toJson() const {
    QJsonObject json;
    TO_JSON_STRING(uuid, uuid);
    TO_JSON_INT(syncStatus, syncStatus);
    TO_JSON_STRING(lastModified, lastModified);
    TO_JSON_STRING(createdAt, createdAt);
    TO_JSON_BOOL(enabled, enabled);
    TO_JSON_INT(cycleMode, cycleMode);
    TO_JSON_STRING(cycleData, cycleData);
    TO_JSON_STRING(time, time);
    TO_JSON_STRING(rangeStart, rangeStart);
    TO_JSON_STRING(rangeEnd, rangeEnd);
    TO_JSON_INT(shutdownOption, shutdownOption);
    TO_JSON_INT(advanceSeconds, advanceSeconds);
    TO_JSON_STRING(label, label);
    return json;
}

ShutdownTask ShutdownTask::fromJson(const QJsonObject& json) {
    ShutdownTask t;
    FROM_JSON_STRING(uuid, t.uuid);
    FROM_JSON_INT(syncStatus, t.syncStatus);
    FROM_JSON_STRING(lastModified, t.lastModified);
    FROM_JSON_STRING(createdAt, t.createdAt);
    FROM_JSON_BOOL(enabled, t.enabled);
    FROM_JSON_INT(cycleMode, t.cycleMode);
    FROM_JSON_STRING(cycleData, t.cycleData);
    FROM_JSON_STRING(time, t.time);
    FROM_JSON_STRING(rangeStart, t.rangeStart);
    FROM_JSON_STRING(rangeEnd, t.rangeEnd);
    FROM_JSON_INT(shutdownOption, t.shutdownOption);
    FROM_JSON_INT(advanceSeconds, t.advanceSeconds);
    FROM_JSON_STRING(label, t.label);
    return t;
}

// ── RunProgramTask ──
QJsonObject RunProgramTask::toJson() const {
    QJsonObject json;
    TO_JSON_STRING(uuid, uuid);
    TO_JSON_INT(syncStatus, syncStatus);
    TO_JSON_STRING(lastModified, lastModified);
    TO_JSON_STRING(createdAt, createdAt);
    TO_JSON_BOOL(enabled, enabled);
    TO_JSON_INT(cycleMode, cycleMode);
    TO_JSON_STRING(cycleData, cycleData);
    TO_JSON_STRING(time, time);
    TO_JSON_STRING(rangeStart, rangeStart);
    TO_JSON_STRING(rangeEnd, rangeEnd);
    TO_JSON_STRING(programPath, programPath);
    TO_JSON_STRING(arguments, arguments);
    TO_JSON_BOOL(ringEnabled, ringEnabled);
    TO_JSON_STRING(label, label);
    return json;
}

RunProgramTask RunProgramTask::fromJson(const QJsonObject& json) {
    RunProgramTask t;
    FROM_JSON_STRING(uuid, t.uuid);
    FROM_JSON_INT(syncStatus, t.syncStatus);
    FROM_JSON_STRING(lastModified, t.lastModified);
    FROM_JSON_STRING(createdAt, t.createdAt);
    FROM_JSON_BOOL(enabled, t.enabled);
    FROM_JSON_INT(cycleMode, t.cycleMode);
    FROM_JSON_STRING(cycleData, t.cycleData);
    FROM_JSON_STRING(time, t.time);
    FROM_JSON_STRING(rangeStart, t.rangeStart);
    FROM_JSON_STRING(rangeEnd, t.rangeEnd);
    FROM_JSON_STRING(programPath, t.programPath);
    FROM_JSON_STRING(arguments, t.arguments);
    FROM_JSON_BOOL(ringEnabled, t.ringEnabled);
    FROM_JSON_STRING(label, t.label);
    return t;
}

// ── Countdown ──
QJsonObject Countdown::toJson() const {
    QJsonObject json;
    TO_JSON_STRING(uuid, uuid);
    TO_JSON_INT(syncStatus, syncStatus);
    TO_JSON_STRING(lastModified, lastModified);
    TO_JSON_STRING(createdAt, createdAt);
    TO_JSON_BOOL(enabled, enabled);
    TO_JSON_INT(mode, mode);
    TO_JSON_INT(totalSeconds, totalSeconds);
    TO_JSON_STRING(targetDatetime, targetDatetime);
    TO_JSON_INT(remainingSeconds, remainingSeconds);
    TO_JSON_INT(ringtone, ringtone);
    TO_JSON_STRING(customRingtonePath, customRingtonePath);
    TO_JSON_INT(ringMode, ringMode);
    TO_JSON_INT(customMinutes, customMinutes);
    TO_JSON_STRING(label, label);
    return json;
}

Countdown Countdown::fromJson(const QJsonObject& json) {
    Countdown c;
    FROM_JSON_STRING(uuid, c.uuid);
    FROM_JSON_INT(syncStatus, c.syncStatus);
    FROM_JSON_STRING(lastModified, c.lastModified);
    FROM_JSON_STRING(createdAt, c.createdAt);
    FROM_JSON_BOOL(enabled, c.enabled);
    FROM_JSON_INT(mode, c.mode);
    FROM_JSON_INT(totalSeconds, c.totalSeconds);
    FROM_JSON_STRING(targetDatetime, c.targetDatetime);
    FROM_JSON_INT(remainingSeconds, c.remainingSeconds);
    FROM_JSON_INT(ringtone, c.ringtone);
    FROM_JSON_STRING(customRingtonePath, c.customRingtonePath);
    FROM_JSON_INT(ringMode, c.ringMode);
    FROM_JSON_INT(customMinutes, c.customMinutes);
    FROM_JSON_STRING(label, c.label);
    return c;
}

// ── HealthSettings ──
QJsonObject HealthSettings::toJson() const {
    QJsonObject json;
    TO_JSON_STRING(uuid, uuid);
    TO_JSON_INT(syncStatus, syncStatus);
    TO_JSON_STRING(lastModified, lastModified);
    TO_JSON_STRING(createdAt, createdAt);
    TO_JSON_BOOL(enabled, enabled);
    TO_JSON_INT(displayMode, displayMode);
    TO_JSON_INT(workMinutes, workMinutes);
    TO_JSON_INT(restMinutes, restMinutes);
    TO_JSON_INT(ringtone, ringtone);
    TO_JSON_STRING(customRingtonePath, customRingtonePath);
    TO_JSON_INT(ringMode, ringMode);
    TO_JSON_INT(customMinutes, customMinutes);
    TO_JSON_STRING(label, label);
    return json;
}

HealthSettings HealthSettings::fromJson(const QJsonObject& json) {
    HealthSettings h;
    FROM_JSON_STRING(uuid, h.uuid);
    FROM_JSON_INT(syncStatus, h.syncStatus);
    FROM_JSON_STRING(lastModified, h.lastModified);
    FROM_JSON_STRING(createdAt, h.createdAt);
    FROM_JSON_BOOL(enabled, h.enabled);
    FROM_JSON_INT(displayMode, h.displayMode);
    FROM_JSON_INT(workMinutes, h.workMinutes);
    FROM_JSON_INT(restMinutes, h.restMinutes);
    FROM_JSON_INT(ringtone, h.ringtone);
    FROM_JSON_STRING(customRingtonePath, h.customRingtonePath);
    FROM_JSON_INT(ringMode, h.ringMode);
    FROM_JSON_INT(customMinutes, h.customMinutes);
    FROM_JSON_STRING(label, h.label);
    return h;
}

#undef TO_JSON_STRING
#undef TO_JSON_INT
#undef TO_JSON_BOOL
#undef FROM_JSON_STRING
#undef FROM_JSON_INT
#undef FROM_JSON_BOOL

} // namespace mcclock::models
