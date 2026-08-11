#include "countdown_dao.h"
#include "database.h"
#include "sqlite3.h"

#include <QMutexLocker>

namespace mcclock::dal {

// ── CountdownDao ──

bool CountdownDao::insert(const models::Countdown& c) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "INSERT INTO countdowns (uuid, sync_status, last_modified, created_at, "
        "enabled, mode, total_seconds, target_datetime, remaining_seconds, ringtone, "
        "custom_ringtone_path, ring_mode, custom_minutes, label) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, c.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, c.syncStatus);
    sqlite3_bind_text(stmt, 3, c.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, c.createdAt.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, c.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 6, c.mode);
    sqlite3_bind_int(stmt, 7, c.totalSeconds);
    sqlite3_bind_text(stmt, 8, c.targetDatetime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, c.remainingSeconds);
    sqlite3_bind_int(stmt, 10, c.ringtone);
    sqlite3_bind_text(stmt, 11, c.customRingtonePath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 12, c.ringMode);
    sqlite3_bind_int(stmt, 13, c.customMinutes);
    sqlite3_bind_text(stmt, 14, c.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool CountdownDao::update(const models::Countdown& c) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "UPDATE countdowns SET sync_status=?, last_modified=?, enabled=?, mode=?, "
        "total_seconds=?, target_datetime=?, remaining_seconds=?, ringtone=?, "
        "custom_ringtone_path=?, ring_mode=?, custom_minutes=?, label=? WHERE uuid=?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, c.syncStatus);
    sqlite3_bind_text(stmt, 2, c.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, c.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 4, c.mode);
    sqlite3_bind_int(stmt, 5, c.totalSeconds);
    sqlite3_bind_text(stmt, 6, c.targetDatetime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, c.remainingSeconds);
    sqlite3_bind_int(stmt, 8, c.ringtone);
    sqlite3_bind_text(stmt, 9, c.customRingtonePath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, c.ringMode);
    sqlite3_bind_int(stmt, 11, c.customMinutes);
    sqlite3_bind_text(stmt, 12, c.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, c.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool CountdownDao::remove(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "DELETE FROM countdowns WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

models::Countdown CountdownDao::rowToCountdown(sqlite3_stmt* stmt) {
    models::Countdown c;
    auto colText = [&](int i) {
        const unsigned char* t = sqlite3_column_text(stmt, i);
        return t ? QString::fromUtf8(reinterpret_cast<const char*>(t)) : QString();
    };
    c.uuid = colText(0);
    c.syncStatus = sqlite3_column_int(stmt, 1);
    c.lastModified = colText(2);
    c.createdAt = colText(3);
    c.enabled = sqlite3_column_int(stmt, 4) != 0;
    c.mode = sqlite3_column_int(stmt, 5);
    c.totalSeconds = sqlite3_column_int(stmt, 6);
    c.targetDatetime = colText(7);
    c.remainingSeconds = sqlite3_column_int(stmt, 8);
    c.ringtone = sqlite3_column_int(stmt, 9);
    c.customRingtonePath = colText(10);
    c.ringMode = sqlite3_column_int(stmt, 11);
    c.customMinutes = sqlite3_column_int(stmt, 12);
    c.label = colText(13);
    return c;
}

static const char* COUNTDOWN_COLS = "uuid, sync_status, last_modified, created_at, enabled, mode, "
    "total_seconds, target_datetime, remaining_seconds, ringtone, custom_ringtone_path, "
    "ring_mode, custom_minutes, label";

models::Countdown CountdownDao::findByUuid(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    models::Countdown c;
    sqlite3* db = Database::instance().handle();
    if (!db) return c;
    QString sql = QString("SELECT %1 FROM countdowns WHERE uuid=?").arg(COUNTDOWN_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return c;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) c = rowToCountdown(stmt);
    sqlite3_finalize(stmt);
    return c;
}

QList<models::Countdown> CountdownDao::findAll() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::Countdown> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM countdowns ORDER BY created_at DESC").arg(COUNTDOWN_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToCountdown(stmt));
    sqlite3_finalize(stmt);
    return list;
}

QList<models::Countdown> CountdownDao::findUnsynced() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::Countdown> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM countdowns WHERE sync_status != 1").arg(COUNTDOWN_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToCountdown(stmt));
    sqlite3_finalize(stmt);
    return list;
}

// ── HealthSettingsDao ──

bool HealthSettingsDao::upsert(const models::HealthSettings& h) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "INSERT INTO health_settings (uuid, sync_status, last_modified, created_at, "
        "enabled, display_mode, work_minutes, rest_minutes, ringtone, custom_ringtone_path, "
        "ring_mode, custom_minutes, label) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(uuid) DO UPDATE SET sync_status=excluded.sync_status, "
        "last_modified=excluded.last_modified, enabled=excluded.enabled, "
        "display_mode=excluded.display_mode, work_minutes=excluded.work_minutes, "
        "rest_minutes=excluded.rest_minutes, ringtone=excluded.ringtone, "
        "custom_ringtone_path=excluded.custom_ringtone_path, ring_mode=excluded.ring_mode, "
        "custom_minutes=excluded.custom_minutes, label=excluded.label";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, h.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, h.syncStatus);
    sqlite3_bind_text(stmt, 3, h.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, h.createdAt.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, h.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 6, h.displayMode);
    sqlite3_bind_int(stmt, 7, h.workMinutes);
    sqlite3_bind_int(stmt, 8, h.restMinutes);
    sqlite3_bind_int(stmt, 9, h.ringtone);
    sqlite3_bind_text(stmt, 10, h.customRingtonePath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 11, h.ringMode);
    sqlite3_bind_int(stmt, 12, h.customMinutes);
    sqlite3_bind_text(stmt, 13, h.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool HealthSettingsDao::remove(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "DELETE FROM health_settings WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

models::HealthSettings HealthSettingsDao::rowToSettings(sqlite3_stmt* stmt) {
    models::HealthSettings h;
    auto colText = [&](int i) {
        const unsigned char* t = sqlite3_column_text(stmt, i);
        return t ? QString::fromUtf8(reinterpret_cast<const char*>(t)) : QString();
    };
    h.uuid = colText(0);
    h.syncStatus = sqlite3_column_int(stmt, 1);
    h.lastModified = colText(2);
    h.createdAt = colText(3);
    h.enabled = sqlite3_column_int(stmt, 4) != 0;
    h.displayMode = sqlite3_column_int(stmt, 5);
    h.workMinutes = sqlite3_column_int(stmt, 6);
    h.restMinutes = sqlite3_column_int(stmt, 7);
    h.ringtone = sqlite3_column_int(stmt, 8);
    h.customRingtonePath = colText(9);
    h.ringMode = sqlite3_column_int(stmt, 10);
    h.customMinutes = sqlite3_column_int(stmt, 11);
    h.label = colText(12);
    return h;
}

static const char* HEALTH_COLS = "uuid, sync_status, last_modified, created_at, enabled, "
    "display_mode, work_minutes, rest_minutes, ringtone, custom_ringtone_path, ring_mode, "
    "custom_minutes, label";

models::HealthSettings HealthSettingsDao::findActive() {
    QMutexLocker lock(&Database::instance().mutex());
    models::HealthSettings h;
    sqlite3* db = Database::instance().handle();
    if (!db) return h;
    QString sql = QString("SELECT %1 FROM health_settings ORDER BY last_modified DESC LIMIT 1").arg(HEALTH_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return h;
    if (sqlite3_step(stmt) == SQLITE_ROW) h = rowToSettings(stmt);
    sqlite3_finalize(stmt);
    return h;
}

QList<models::HealthSettings> HealthSettingsDao::findAll() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::HealthSettings> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM health_settings ORDER BY created_at DESC").arg(HEALTH_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToSettings(stmt));
    sqlite3_finalize(stmt);
    return list;
}

QList<models::HealthSettings> HealthSettingsDao::findUnsynced() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::HealthSettings> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM health_settings WHERE sync_status != 1").arg(HEALTH_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToSettings(stmt));
    sqlite3_finalize(stmt);
    return list;
}

} // namespace mcclock::dal
