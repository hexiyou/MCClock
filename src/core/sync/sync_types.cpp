#include "sync_types.h"

namespace mcclock::sync {

QJsonObject SyncPayload::toJson() const {
    QJsonObject json;
    json["client_id"] = clientId;
    json["timestamp"] = timestamp;
    json["schema_version"] = schemaVersion;

    QJsonArray alarmArr;
    for (const auto& a : alarms) alarmArr.append(a.toJson());
    json["alarms"] = alarmArr;

    QJsonArray groupArr;
    for (const auto& g : groups) groupArr.append(g.toJson());
    json["groups"] = groupArr;

    QJsonArray bdayArr;
    for (const auto& b : birthdays) bdayArr.append(b.toJson());
    json["birthdays"] = bdayArr;

    QJsonArray sdArr;
    for (const auto& t : shutdownTasks) sdArr.append(t.toJson());
    json["shutdown_tasks"] = sdArr;

    QJsonArray rpArr;
    for (const auto& t : runProgramTasks) rpArr.append(t.toJson());
    json["run_program_tasks"] = rpArr;

    QJsonArray cdArr;
    for (const auto& c : countdowns) cdArr.append(c.toJson());
    json["countdowns"] = cdArr;

    QJsonArray hsArr;
    for (const auto& h : healthSettings) hsArr.append(h.toJson());
    json["health_settings"] = hsArr;

    return json;
}

SyncPayload SyncPayload::fromJson(const QJsonObject& json) {
    SyncPayload p;
    p.clientId = json["client_id"].toString();
    p.timestamp = json["timestamp"].toString();
    p.schemaVersion = json["schema_version"].toInt(1);

    for (const auto& v : json["alarms"].toArray())
        p.alarms.append(models::Alarm::fromJson(v.toObject()));
    for (const auto& v : json["groups"].toArray())
        p.groups.append(models::AlarmGroup::fromJson(v.toObject()));
    for (const auto& v : json["birthdays"].toArray())
        p.birthdays.append(models::Birthday::fromJson(v.toObject()));
    for (const auto& v : json["shutdown_tasks"].toArray())
        p.shutdownTasks.append(models::ShutdownTask::fromJson(v.toObject()));
    for (const auto& v : json["run_program_tasks"].toArray())
        p.runProgramTasks.append(models::RunProgramTask::fromJson(v.toObject()));
    for (const auto& v : json["countdowns"].toArray())
        p.countdowns.append(models::Countdown::fromJson(v.toObject()));
    for (const auto& v : json["health_settings"].toArray())
        p.healthSettings.append(models::HealthSettings::fromJson(v.toObject()));

    return p;
}

QJsonObject SyncResponse::toJson() const {
    QJsonObject json;
    json["success"] = success;
    json["server_timestamp"] = serverTimestamp;
    json["error_message"] = errorMessage;
    return json;
}

SyncResponse SyncResponse::fromJson(const QJsonObject& json) {
    SyncResponse r;
    r.success = json["success"].toBool();
    r.serverTimestamp = json["server_timestamp"].toString();
    r.errorMessage = json["error_message"].toString();
    return r;
}

} // namespace mcclock::sync
