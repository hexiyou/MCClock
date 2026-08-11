#include "alarm_dao.h"
#include "database.h"
#include "sqlite3.h"
#include "core/utils/uuid_utils.h"
#include "core/utils/time_utils.h"

#include <QMutexLocker>

namespace mcclock::dal {

// ── AlarmDao ──

bool AlarmDao::insert(const models::Alarm& alarm) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "INSERT INTO alarms (uuid, sync_status, last_modified, created_at, "
        "enabled, cycle_mode, cycle_data, time, range_start, range_end, ringtone, "
        "custom_ringtone_path, ring_mode, custom_minutes, label, group_id, deleted) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, alarm.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, alarm.syncStatus);
    sqlite3_bind_text(stmt, 3, alarm.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, alarm.createdAt.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, alarm.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 6, alarm.cycleMode);
    sqlite3_bind_text(stmt, 7, alarm.cycleData.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, alarm.time.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, alarm.rangeStart.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, alarm.rangeEnd.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 11, alarm.ringtone);
    sqlite3_bind_text(stmt, 12, alarm.customRingtonePath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 13, alarm.ringMode);
    sqlite3_bind_int(stmt, 14, alarm.customMinutes);
    sqlite3_bind_text(stmt, 15, alarm.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 16, alarm.groupId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 17, alarm.deleted ? 1 : 0);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool AlarmDao::update(const models::Alarm& alarm) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "UPDATE alarms SET sync_status=?, last_modified=?, enabled=?, "
        "cycle_mode=?, cycle_data=?, time=?, range_start=?, range_end=?, ringtone=?, "
        "custom_ringtone_path=?, ring_mode=?, custom_minutes=?, label=?, group_id=?, deleted=? "
        "WHERE uuid=?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, alarm.syncStatus);
    sqlite3_bind_text(stmt, 2, alarm.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, alarm.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 4, alarm.cycleMode);
    sqlite3_bind_text(stmt, 5, alarm.cycleData.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, alarm.time.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, alarm.rangeStart.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, alarm.rangeEnd.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, alarm.ringtone);
    sqlite3_bind_text(stmt, 10, alarm.customRingtonePath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 11, alarm.ringMode);
    sqlite3_bind_int(stmt, 12, alarm.customMinutes);
    sqlite3_bind_text(stmt, 13, alarm.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 14, alarm.groupId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 15, alarm.deleted ? 1 : 0);
    sqlite3_bind_text(stmt, 16, alarm.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool AlarmDao::remove(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "UPDATE alarms SET deleted=1, sync_status=2, last_modified=? WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    QString now = utils::TimeUtils::nowISO8601();
    sqlite3_bind_text(stmt, 1, now.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool AlarmDao::hardDelete(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "DELETE FROM alarms WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool AlarmDao::restore(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "UPDATE alarms SET deleted=0, sync_status=2, last_modified=? WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    QString now = utils::TimeUtils::nowISO8601();
    sqlite3_bind_text(stmt, 1, now.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

models::Alarm AlarmDao::rowToAlarm(sqlite3_stmt* stmt) {
    models::Alarm a;
    auto colText = [&](int i) {
        const unsigned char* t = sqlite3_column_text(stmt, i);
        return t ? QString::fromUtf8(reinterpret_cast<const char*>(t)) : QString();
    };
    a.uuid = colText(0);
    a.syncStatus = sqlite3_column_int(stmt, 1);
    a.lastModified = colText(2);
    a.createdAt = colText(3);
    a.enabled = sqlite3_column_int(stmt, 4) != 0;
    a.cycleMode = sqlite3_column_int(stmt, 5);
    a.cycleData = colText(6);
    a.time = colText(7);
    a.rangeStart = colText(8);
    a.rangeEnd = colText(9);
    a.ringtone = sqlite3_column_int(stmt, 10);
    a.customRingtonePath = colText(11);
    a.ringMode = sqlite3_column_int(stmt, 12);
    a.customMinutes = sqlite3_column_int(stmt, 13);
    a.label = colText(14);
    a.groupId = colText(15);
    a.deleted = sqlite3_column_int(stmt, 16) != 0;
    return a;
}

static const char* ALARM_COLS = "uuid, sync_status, last_modified, created_at, enabled, "
    "cycle_mode, cycle_data, time, range_start, range_end, ringtone, custom_ringtone_path, "
    "ring_mode, custom_minutes, label, group_id, deleted";

models::Alarm AlarmDao::findByUuid(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    models::Alarm a;
    sqlite3* db = Database::instance().handle();
    if (!db) return a;
    QString sql = QString("SELECT %1 FROM alarms WHERE uuid=?").arg(ALARM_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return a;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) a = rowToAlarm(stmt);
    sqlite3_finalize(stmt);
    return a;
}

QList<models::Alarm> AlarmDao::findAll(bool includeDeleted) {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::Alarm> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM alarms").arg(ALARM_COLS);
    if (!includeDeleted) sql += " WHERE deleted=0";
    sql += " ORDER BY created_at DESC";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToAlarm(stmt));
    sqlite3_finalize(stmt);
    return list;
}

QList<models::Alarm> AlarmDao::findByGroup(const QString& groupId) {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::Alarm> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM alarms WHERE deleted=0 AND group_id=? ORDER BY created_at DESC").arg(ALARM_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    sqlite3_bind_text(stmt, 1, groupId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToAlarm(stmt));
    sqlite3_finalize(stmt);
    return list;
}

QList<models::Alarm> AlarmDao::findUnsynced() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::Alarm> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM alarms WHERE sync_status != 1").arg(ALARM_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToAlarm(stmt));
    sqlite3_finalize(stmt);
    return list;
}

bool AlarmDao::clearRecycleBin() {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "DELETE FROM alarms WHERE deleted=1";
    return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

QList<models::Alarm> AlarmDao::findDeleted() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::Alarm> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM alarms WHERE deleted=1 ORDER BY created_at DESC").arg(ALARM_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToAlarm(stmt));
    sqlite3_finalize(stmt);
    return list;
}

// ── AlarmGroupDao ──

bool AlarmGroupDao::insert(const models::AlarmGroup& group) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "INSERT INTO alarm_groups (uuid, sync_status, last_modified, created_at, name, sort_order) VALUES (?,?,?,?,?,?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, group.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, group.syncStatus);
    sqlite3_bind_text(stmt, 3, group.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, group.createdAt.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, group.name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, group.sortOrder);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool AlarmGroupDao::update(const models::AlarmGroup& group) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "UPDATE alarm_groups SET sync_status=?, last_modified=?, name=?, sort_order=? WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, group.syncStatus);
    sqlite3_bind_text(stmt, 2, group.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, group.name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, group.sortOrder);
    sqlite3_bind_text(stmt, 5, group.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool AlarmGroupDao::remove(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "DELETE FROM alarm_groups WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

models::AlarmGroup AlarmGroupDao::rowToGroup(sqlite3_stmt* stmt) {
    models::AlarmGroup g;
    auto colText = [&](int i) {
        const unsigned char* t = sqlite3_column_text(stmt, i);
        return t ? QString::fromUtf8(reinterpret_cast<const char*>(t)) : QString();
    };
    g.uuid = colText(0);
    g.syncStatus = sqlite3_column_int(stmt, 1);
    g.lastModified = colText(2);
    g.createdAt = colText(3);
    g.name = colText(4);
    g.sortOrder = sqlite3_column_int(stmt, 5);
    return g;
}

models::AlarmGroup AlarmGroupDao::findByUuid(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    models::AlarmGroup g;
    sqlite3* db = Database::instance().handle();
    if (!db) return g;
    const char* sql = "SELECT uuid, sync_status, last_modified, created_at, name, sort_order FROM alarm_groups WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return g;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) g = rowToGroup(stmt);
    sqlite3_finalize(stmt);
    return g;
}

QList<models::AlarmGroup> AlarmGroupDao::findAll() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::AlarmGroup> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    const char* sql = "SELECT uuid, sync_status, last_modified, created_at, name, sort_order FROM alarm_groups ORDER BY sort_order";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToGroup(stmt));
    sqlite3_finalize(stmt);
    return list;
}

} // namespace mcclock::dal
