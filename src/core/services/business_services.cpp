#include "business_services.h"
#include "lunar_calendar.h"
#include "cycle_utils.h"
#include "core/dal/alarm_dao.h"
#include "core/dal/birthday_dao.h"
#include "core/dal/task_dao.h"
#include "core/dal/countdown_dao.h"
#include "core/utils/uuid_utils.h"
#include "core/utils/time_utils.h"
#include "core/utils/platform_utils.h"

#include <QDate>

namespace mcclock::services {

// ── helpers ──
static void stampNew(models::Alarm& a) {
    if (a.uuid.isEmpty()) a.uuid = utils::generateUuid();
    QString now = utils::TimeUtils::nowISO8601();
    if (a.createdAt.isEmpty()) a.createdAt = now;
    a.lastModified = now;
}

// ── AlarmService ──

models::Alarm AlarmService::add(models::Alarm alarm) {
    stampNew(alarm);
    alarm.syncStatus = static_cast<int>(models::SyncStatus::New);
    dal::AlarmDao dao;
    if (!dao.insert(alarm)) alarm.uuid.clear();
    return alarm;
}

bool AlarmService::update(models::Alarm alarm) {
    alarm.lastModified = utils::TimeUtils::nowISO8601();
    alarm.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    dal::AlarmDao dao;
    return dao.update(alarm);
}

bool AlarmService::moveToRecycleBin(const QString& uuid) {
    return dal::AlarmDao().remove(uuid);
}

bool AlarmService::restore(const QString& uuid) {
    return dal::AlarmDao().restore(uuid);
}

bool AlarmService::hardDelete(const QString& uuid) {
    return dal::AlarmDao().hardDelete(uuid);
}

bool AlarmService::clearRecycleBin() {
    return dal::AlarmDao().clearRecycleBin();
}

bool AlarmService::setEnabled(const QString& uuid, bool enabled) {
    dal::AlarmDao dao;
    models::Alarm a = dao.findByUuid(uuid);
    if (a.uuid.isEmpty()) return false;
    a.enabled = enabled;
    a.lastModified = utils::TimeUtils::nowISO8601();
    a.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    return dao.update(a);
}

models::Alarm AlarmService::findByUuid(const QString& uuid) {
    return dal::AlarmDao().findByUuid(uuid);
}

QList<models::Alarm> AlarmService::findAll() {
    return dal::AlarmDao().findAll();
}

QList<models::Alarm> AlarmService::findByGroup(const QString& groupId) {
    return dal::AlarmDao().findByGroup(groupId);
}

QList<models::Alarm> AlarmService::findDeleted() {
    return dal::AlarmDao().findDeleted();
}

QList<models::Alarm> AlarmService::findMissed(const QDateTime& lastRunTime) {
    // An alarm is "missed" if it is enabled and its previous occurrence
    // falls between lastRunTime and now.
    QList<models::Alarm> missed;
    QDateTime now = QDateTime::currentDateTime();
    if (!lastRunTime.isValid() || lastRunTime >= now) return missed;

    const auto all = dal::AlarmDao().findAll();
    for (const auto& a : all) {
        if (!a.enabled) continue;
        // Check every minute-boundary occurrence in (lastRunTime, now]
        QDateTime probe = lastRunTime.addSecs(60);
        probe.setTime(QTime(probe.time().hour(), probe.time().minute(), 0));
        if (probe <= lastRunTime) probe = probe.addSecs(60);
        QTime alarmTime = QTime::fromString(a.time, "HH:mm");
        if (!alarmTime.isValid()) continue;

        for (QDateTime p = probe; p <= now; p = p.addSecs(60)) {
            QDate d = p.date();
            QTime pt = p.time();
            if (pt.hour() != alarmTime.hour() || pt.minute() != alarmTime.minute()) continue;
            if (CycleUtils::occursOnDate(d, a.cycleMode, a.cycleData)) {
                missed.append(a);
                break;
            }
        }
    }
    return missed;
}

// ── BirthdayService ──

models::Birthday BirthdayService::add(models::Birthday b) {
    if (b.uuid.isEmpty()) b.uuid = utils::generateUuid();
    QString now = utils::TimeUtils::nowISO8601();
    if (b.createdAt.isEmpty()) b.createdAt = now;
    b.lastModified = now;
    b.syncStatus = static_cast<int>(models::SyncStatus::New);
    if (!dal::BirthdayDao().insert(b)) b.uuid.clear();
    return b;
}

bool BirthdayService::update(models::Birthday b) {
    b.lastModified = utils::TimeUtils::nowISO8601();
    b.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    return dal::BirthdayDao().update(b);
}

bool BirthdayService::remove(const QString& uuid) {
    return dal::BirthdayDao().remove(uuid);
}

models::Birthday BirthdayService::findByUuid(const QString& uuid) {
    return dal::BirthdayDao().findByUuid(uuid);
}

QList<models::Birthday> BirthdayService::findAll() {
    return dal::BirthdayDao().findAll();
}

QDate BirthdayService::nextBirthdayDate(const models::Birthday& b) {
    QDate today = QDate::currentDate();

    if (!b.isLunar) {
        // Gregorian: try this year then next year (handles Feb 29)
        for (int y : { today.year(), today.year() + 1 }) {
            QDate cand(y, b.solarMonth, 1);
            if (b.solarDay > cand.daysInMonth()) continue; // skip invalid (Feb 29 on non-leap)
            cand.setDate(y, b.solarMonth, b.solarDay);
            if (cand >= today) return cand;
        }
        return QDate();
    }

    // Lunar: convert lunar month/day to solar for this year and next
    for (int y : { today.year(), today.year() + 1 }) {
        int sy = 0, sm = 0, sd = 0;
        LunarCalendar::lunarToSolar(y, b.lunarMonth, b.lunarDay, false, sy, sm, sd);
        if (sy == 0) continue;
        QDate cand(sy, sm, sd);
        if (cand.isValid() && cand >= today) return cand;
    }
    return QDate();
}

int BirthdayService::daysUntilBirthday(const models::Birthday& b) {
    QDate next = nextBirthdayDate(b);
    if (!next.isValid()) return -1;
    return QDate::currentDate().daysTo(next);
}

// ── ShutdownService ──

models::ShutdownTask ShutdownService::add(models::ShutdownTask t) {
    if (t.uuid.isEmpty()) t.uuid = utils::generateUuid();
    QString now = utils::TimeUtils::nowISO8601();
    if (t.createdAt.isEmpty()) t.createdAt = now;
    t.lastModified = now;
    t.syncStatus = static_cast<int>(models::SyncStatus::New);
    if (!dal::ShutdownTaskDao().insert(t)) t.uuid.clear();
    return t;
}

bool ShutdownService::update(models::ShutdownTask t) {
    t.lastModified = utils::TimeUtils::nowISO8601();
    t.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    return dal::ShutdownTaskDao().update(t);
}

bool ShutdownService::remove(const QString& uuid) {
    return dal::ShutdownTaskDao().remove(uuid);
}

bool ShutdownService::setEnabled(const QString& uuid, bool enabled) {
    dal::ShutdownTaskDao dao;
    models::ShutdownTask t = dao.findByUuid(uuid);
    if (t.uuid.isEmpty()) return false;
    t.enabled = enabled;
    t.lastModified = utils::TimeUtils::nowISO8601();
    t.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    return dao.update(t);
}

models::ShutdownTask ShutdownService::findByUuid(const QString& uuid) {
    return dal::ShutdownTaskDao().findByUuid(uuid);
}

QList<models::ShutdownTask> ShutdownService::findAll() {
    return dal::ShutdownTaskDao().findAll();
}

bool ShutdownService::executeNow(const models::ShutdownTask& t) {
    return utils::PlatformUtils::executeShutdown(t.shutdownOption);
}

// ── RunProgramService ──

models::RunProgramTask RunProgramService::add(models::RunProgramTask t) {
    if (t.uuid.isEmpty()) t.uuid = utils::generateUuid();
    QString now = utils::TimeUtils::nowISO8601();
    if (t.createdAt.isEmpty()) t.createdAt = now;
    t.lastModified = now;
    t.syncStatus = static_cast<int>(models::SyncStatus::New);
    if (!dal::RunProgramTaskDao().insert(t)) t.uuid.clear();
    return t;
}

bool RunProgramService::update(models::RunProgramTask t) {
    t.lastModified = utils::TimeUtils::nowISO8601();
    t.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    return dal::RunProgramTaskDao().update(t);
}

bool RunProgramService::remove(const QString& uuid) {
    return dal::RunProgramTaskDao().remove(uuid);
}

bool RunProgramService::setEnabled(const QString& uuid, bool enabled) {
    dal::RunProgramTaskDao dao;
    models::RunProgramTask t = dao.findByUuid(uuid);
    if (t.uuid.isEmpty()) return false;
    t.enabled = enabled;
    t.lastModified = utils::TimeUtils::nowISO8601();
    t.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    return dao.update(t);
}

models::RunProgramTask RunProgramService::findByUuid(const QString& uuid) {
    return dal::RunProgramTaskDao().findByUuid(uuid);
}

QList<models::RunProgramTask> RunProgramService::findAll() {
    return dal::RunProgramTaskDao().findAll();
}

bool RunProgramService::executeNow(const models::RunProgramTask& t) {
    return utils::PlatformUtils::runProgramOrUrl(t.programPath, t.arguments);
}

bool RunProgramService::isUrl(const QString& path) {
    return path.startsWith("http://") || path.startsWith("https://");
}

// ── CountdownService ──

models::Countdown CountdownService::add(models::Countdown c) {
    if (c.uuid.isEmpty()) c.uuid = utils::generateUuid();
    QString now = utils::TimeUtils::nowISO8601();
    if (c.createdAt.isEmpty()) c.createdAt = now;
    c.lastModified = now;
    c.syncStatus = static_cast<int>(models::SyncStatus::New);
    if (c.mode == static_cast<int>(models::CountdownMode::Relative)) {
        c.remainingSeconds = c.totalSeconds;
    }
    if (!dal::CountdownDao().insert(c)) c.uuid.clear();
    return c;
}

bool CountdownService::update(models::Countdown c) {
    c.lastModified = utils::TimeUtils::nowISO8601();
    c.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    return dal::CountdownDao().update(c);
}

bool CountdownService::remove(const QString& uuid) {
    return dal::CountdownDao().remove(uuid);
}

bool CountdownService::setEnabled(const QString& uuid, bool enabled) {
    dal::CountdownDao dao;
    models::Countdown c = dao.findByUuid(uuid);
    if (c.uuid.isEmpty()) return false;
    c.enabled = enabled;
    c.lastModified = utils::TimeUtils::nowISO8601();
    c.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    return dao.update(c);
}

bool CountdownService::saveRemaining(const QString& uuid, int remainingSeconds) {
    dal::CountdownDao dao;
    models::Countdown c = dao.findByUuid(uuid);
    if (c.uuid.isEmpty()) return false;
    c.remainingSeconds = remainingSeconds;
    return dao.update(c);
}

models::Countdown CountdownService::findByUuid(const QString& uuid) {
    return dal::CountdownDao().findByUuid(uuid);
}

QList<models::Countdown> CountdownService::findAll() {
    return dal::CountdownDao().findAll();
}

// ── HealthService ──

models::HealthSettings HealthService::get() {
    dal::HealthSettingsDao dao;
    models::HealthSettings h = dao.findActive();
    if (h.uuid.isEmpty()) {
        // Create default row
        h.uuid = utils::generateUuid();
        QString now = utils::TimeUtils::nowISO8601();
        h.createdAt = now;
        h.lastModified = now;
        h.syncStatus = static_cast<int>(models::SyncStatus::New);
        dao.upsert(h);
    }
    return h;
}

bool HealthService::save(models::HealthSettings h) {
    if (h.uuid.isEmpty()) {
        h.uuid = utils::generateUuid();
        h.createdAt = utils::TimeUtils::nowISO8601();
    }
    h.lastModified = utils::TimeUtils::nowISO8601();
    h.syncStatus = static_cast<int>(models::SyncStatus::Modified);
    return dal::HealthSettingsDao().upsert(h);
}

} // namespace mcclock::services
