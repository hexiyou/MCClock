#include "sync_manager.h"

#include <QDateTime>
#include <QUuid>

#include "core/dal/alarm_dao.h"
#include "core/dal/birthday_dao.h"
#include "core/dal/task_dao.h"
#include "core/dal/countdown_dao.h"
#include "core/dal/settings_manager.h"

namespace mcclock::sync {

using namespace mcclock::dal;
using namespace mcclock::models;

namespace {

// Stable device identifier persisted in settings ("cloud_sync.client_id")
QString deviceId() {
    auto& s = SettingsManager::instance();
    QString id = s.get({"cloud_sync", "client_id"}).toString();
    if (id.isEmpty()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        s.set({"cloud_sync", "client_id"}, id);
        s.save();
    }
    return id;
}

} // namespace

SyncManager::SyncManager(QObject* parent)
    : QObject(parent) {}

void SyncManager::setApi(std::unique_ptr<ISyncApi> api) {
    api_ = std::move(api);
}

void SyncManager::startAutoSync(int intervalMinutes) {
    if (intervalMinutes <= 0) return;
    if (!autoSyncTimer_) {
        autoSyncTimer_ = new QTimer(this);
        connect(autoSyncTimer_, &QTimer::timeout, this, &SyncManager::syncNow);
    }
    autoSyncTimer_->start(intervalMinutes * 60 * 1000);
}

void SyncManager::stopAutoSync() {
    if (autoSyncTimer_) autoSyncTimer_->stop();
}

SyncPayload SyncManager::collectLocalChanges() const {
    SyncPayload payload;
    payload.clientId = deviceId();
    payload.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    payload.schemaVersion = 1;

    payload.alarms = AlarmDao().findUnsynced();
    payload.birthdays = BirthdayDao().findUnsynced();
    payload.shutdownTasks = ShutdownTaskDao().findUnsynced();
    payload.runProgramTasks = RunProgramTaskDao().findUnsynced();
    payload.countdowns = CountdownDao().findUnsynced();
    payload.healthSettings = HealthSettingsDao().findUnsynced();
    return payload;
}

void SyncManager::applyServerChanges(const SyncResponse& response) {
    // Server wins: upsert each conflict record into the local database
    AlarmDao alarmDao;
    for (const auto& a : response.conflictAlarms) {
        if (alarmDao.findByUuid(a.uuid).uuid.isEmpty()) alarmDao.insert(a);
        else alarmDao.update(a);
    }

    BirthdayDao birthdayDao;
    for (const auto& b : response.conflictBirthdays) {
        if (birthdayDao.findByUuid(b.uuid).uuid.isEmpty()) birthdayDao.insert(b);
        else birthdayDao.update(b);
    }

    ShutdownTaskDao shutdownDao;
    for (const auto& t : response.conflictShutdownTasks) {
        if (shutdownDao.findByUuid(t.uuid).uuid.isEmpty()) shutdownDao.insert(t);
        else shutdownDao.update(t);
    }

    RunProgramTaskDao runDao;
    for (const auto& t : response.conflictRunProgramTasks) {
        if (runDao.findByUuid(t.uuid).uuid.isEmpty()) runDao.insert(t);
        else runDao.update(t);
    }

    CountdownDao countdownDao;
    for (const auto& c : response.conflictCountdowns) {
        if (countdownDao.findByUuid(c.uuid).uuid.isEmpty()) countdownDao.insert(c);
        else countdownDao.update(c);
    }
}

void SyncManager::syncNow() {
    emit syncStarted();

    bool success = false;
    if (api_ && api_->isAuthenticated()) {
        SyncPayload payload = collectLocalChanges();
        SyncResponse response = api_->incrementalSync(payload);
        if (response.success) {
            applyServerChanges(response);
            auto& s = SettingsManager::instance();
            s.setLastSyncTime(QDateTime::currentDateTime().toString(Qt::ISODate));
            s.save();
            success = true;
        }
    }
    // Stub: without an authenticated API the sync is a no-op

    emit syncCompleted(success);
}

} // namespace mcclock::sync
