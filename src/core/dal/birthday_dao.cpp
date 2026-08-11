#include "birthday_dao.h"
#include "database.h"
#include "sqlite3.h"

#include <QMutexLocker>

namespace mcclock::dal {

bool BirthdayDao::insert(const models::Birthday& b) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "INSERT INTO birthdays (uuid, sync_status, last_modified, created_at, "
        "name, gender, is_lunar, solar_year, solar_month, solar_day, lunar_month, lunar_day, "
        "remind_time, advance_days, ringtone, custom_ringtone_path, ring_mode, custom_minutes, "
        "avatar_path, label) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, b.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, b.syncStatus);
    sqlite3_bind_text(stmt, 3, b.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, b.createdAt.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, b.name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, b.gender);
    sqlite3_bind_int(stmt, 7, b.isLunar ? 1 : 0);
    sqlite3_bind_int(stmt, 8, b.solarYear);
    sqlite3_bind_int(stmt, 9, b.solarMonth);
    sqlite3_bind_int(stmt, 10, b.solarDay);
    sqlite3_bind_int(stmt, 11, b.lunarMonth);
    sqlite3_bind_int(stmt, 12, b.lunarDay);
    sqlite3_bind_text(stmt, 13, b.remindTime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 14, b.advanceDays);
    sqlite3_bind_int(stmt, 15, b.ringtone);
    sqlite3_bind_text(stmt, 16, b.customRingtonePath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 17, b.ringMode);
    sqlite3_bind_int(stmt, 18, b.customMinutes);
    sqlite3_bind_text(stmt, 19, b.avatarPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 20, b.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool BirthdayDao::update(const models::Birthday& b) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "UPDATE birthdays SET sync_status=?, last_modified=?, name=?, gender=?, "
        "is_lunar=?, solar_year=?, solar_month=?, solar_day=?, lunar_month=?, lunar_day=?, "
        "remind_time=?, advance_days=?, ringtone=?, custom_ringtone_path=?, ring_mode=?, "
        "custom_minutes=?, avatar_path=?, label=? WHERE uuid=?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, b.syncStatus);
    sqlite3_bind_text(stmt, 2, b.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, b.name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, b.gender);
    sqlite3_bind_int(stmt, 5, b.isLunar ? 1 : 0);
    sqlite3_bind_int(stmt, 6, b.solarYear);
    sqlite3_bind_int(stmt, 7, b.solarMonth);
    sqlite3_bind_int(stmt, 8, b.solarDay);
    sqlite3_bind_int(stmt, 9, b.lunarMonth);
    sqlite3_bind_int(stmt, 10, b.lunarDay);
    sqlite3_bind_text(stmt, 11, b.remindTime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 12, b.advanceDays);
    sqlite3_bind_int(stmt, 13, b.ringtone);
    sqlite3_bind_text(stmt, 14, b.customRingtonePath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 15, b.ringMode);
    sqlite3_bind_int(stmt, 16, b.customMinutes);
    sqlite3_bind_text(stmt, 17, b.avatarPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 18, b.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 19, b.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool BirthdayDao::remove(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "DELETE FROM birthdays WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

models::Birthday BirthdayDao::rowToBirthday(sqlite3_stmt* stmt) {
    models::Birthday b;
    auto colText = [&](int i) {
        const unsigned char* t = sqlite3_column_text(stmt, i);
        return t ? QString::fromUtf8(reinterpret_cast<const char*>(t)) : QString();
    };
    b.uuid = colText(0);
    b.syncStatus = sqlite3_column_int(stmt, 1);
    b.lastModified = colText(2);
    b.createdAt = colText(3);
    b.name = colText(4);
    b.gender = sqlite3_column_int(stmt, 5);
    b.isLunar = sqlite3_column_int(stmt, 6) != 0;
    b.solarYear = sqlite3_column_int(stmt, 7);
    b.solarMonth = sqlite3_column_int(stmt, 8);
    b.solarDay = sqlite3_column_int(stmt, 9);
    b.lunarMonth = sqlite3_column_int(stmt, 10);
    b.lunarDay = sqlite3_column_int(stmt, 11);
    b.remindTime = colText(12);
    b.advanceDays = sqlite3_column_int(stmt, 13);
    b.ringtone = sqlite3_column_int(stmt, 14);
    b.customRingtonePath = colText(15);
    b.ringMode = sqlite3_column_int(stmt, 16);
    b.customMinutes = sqlite3_column_int(stmt, 17);
    b.avatarPath = colText(18);
    b.label = colText(19);
    return b;
}

static const char* BIRTHDAY_COLS = "uuid, sync_status, last_modified, created_at, name, gender, "
    "is_lunar, solar_year, solar_month, solar_day, lunar_month, lunar_day, remind_time, "
    "advance_days, ringtone, custom_ringtone_path, ring_mode, custom_minutes, avatar_path, label";

models::Birthday BirthdayDao::findByUuid(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    models::Birthday b;
    sqlite3* db = Database::instance().handle();
    if (!db) return b;
    QString sql = QString("SELECT %1 FROM birthdays WHERE uuid=?").arg(BIRTHDAY_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return b;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) b = rowToBirthday(stmt);
    sqlite3_finalize(stmt);
    return b;
}

QList<models::Birthday> BirthdayDao::findAll() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::Birthday> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM birthdays ORDER BY created_at DESC").arg(BIRTHDAY_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToBirthday(stmt));
    sqlite3_finalize(stmt);
    return list;
}

QList<models::Birthday> BirthdayDao::findUnsynced() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::Birthday> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM birthdays WHERE sync_status != 1").arg(BIRTHDAY_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToBirthday(stmt));
    sqlite3_finalize(stmt);
    return list;
}

} // namespace mcclock::dal
