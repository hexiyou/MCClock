#include "task_dao.h"
#include "database.h"
#include "sqlite3.h"

#include <QMutexLocker>

namespace mcclock::dal {

// ── ShutdownTaskDao ──

bool ShutdownTaskDao::insert(const models::ShutdownTask& t) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "INSERT INTO shutdown_tasks (uuid, sync_status, last_modified, created_at, "
        "enabled, cycle_mode, cycle_data, time, range_start, range_end, shutdown_option, "
        "advance_seconds, label) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, t.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, t.syncStatus);
    sqlite3_bind_text(stmt, 3, t.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, t.createdAt.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, t.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 6, t.cycleMode);
    sqlite3_bind_text(stmt, 7, t.cycleData.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, t.time.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, t.rangeStart.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, t.rangeEnd.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 11, t.shutdownOption);
    sqlite3_bind_int(stmt, 12, t.advanceSeconds);
    sqlite3_bind_text(stmt, 13, t.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool ShutdownTaskDao::update(const models::ShutdownTask& t) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "UPDATE shutdown_tasks SET sync_status=?, last_modified=?, enabled=?, "
        "cycle_mode=?, cycle_data=?, time=?, range_start=?, range_end=?, shutdown_option=?, "
        "advance_seconds=?, label=? WHERE uuid=?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, t.syncStatus);
    sqlite3_bind_text(stmt, 2, t.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, t.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 4, t.cycleMode);
    sqlite3_bind_text(stmt, 5, t.cycleData.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, t.time.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, t.rangeStart.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, t.rangeEnd.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, t.shutdownOption);
    sqlite3_bind_int(stmt, 10, t.advanceSeconds);
    sqlite3_bind_text(stmt, 11, t.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, t.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool ShutdownTaskDao::remove(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "DELETE FROM shutdown_tasks WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

models::ShutdownTask ShutdownTaskDao::rowToTask(sqlite3_stmt* stmt) {
    models::ShutdownTask t;
    auto colText = [&](int i) {
        const unsigned char* tx = sqlite3_column_text(stmt, i);
        return tx ? QString::fromUtf8(reinterpret_cast<const char*>(tx)) : QString();
    };
    t.uuid = colText(0);
    t.syncStatus = sqlite3_column_int(stmt, 1);
    t.lastModified = colText(2);
    t.createdAt = colText(3);
    t.enabled = sqlite3_column_int(stmt, 4) != 0;
    t.cycleMode = sqlite3_column_int(stmt, 5);
    t.cycleData = colText(6);
    t.time = colText(7);
    t.rangeStart = colText(8);
    t.rangeEnd = colText(9);
    t.shutdownOption = sqlite3_column_int(stmt, 10);
    t.advanceSeconds = sqlite3_column_int(stmt, 11);
    t.label = colText(12);
    return t;
}

static const char* SHUTDOWN_COLS = "uuid, sync_status, last_modified, created_at, enabled, "
    "cycle_mode, cycle_data, time, range_start, range_end, shutdown_option, advance_seconds, label";

models::ShutdownTask ShutdownTaskDao::findByUuid(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    models::ShutdownTask t;
    sqlite3* db = Database::instance().handle();
    if (!db) return t;
    QString sql = QString("SELECT %1 FROM shutdown_tasks WHERE uuid=?").arg(SHUTDOWN_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return t;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) t = rowToTask(stmt);
    sqlite3_finalize(stmt);
    return t;
}

QList<models::ShutdownTask> ShutdownTaskDao::findAll() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::ShutdownTask> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM shutdown_tasks ORDER BY created_at DESC").arg(SHUTDOWN_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToTask(stmt));
    sqlite3_finalize(stmt);
    return list;
}

QList<models::ShutdownTask> ShutdownTaskDao::findUnsynced() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::ShutdownTask> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM shutdown_tasks WHERE sync_status != 1").arg(SHUTDOWN_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToTask(stmt));
    sqlite3_finalize(stmt);
    return list;
}

// ── RunProgramTaskDao ──

bool RunProgramTaskDao::insert(const models::RunProgramTask& t) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "INSERT INTO run_program_tasks (uuid, sync_status, last_modified, created_at, "
        "enabled, cycle_mode, cycle_data, time, range_start, range_end, program_path, arguments, "
        "ring_enabled, label) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, t.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, t.syncStatus);
    sqlite3_bind_text(stmt, 3, t.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, t.createdAt.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, t.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 6, t.cycleMode);
    sqlite3_bind_text(stmt, 7, t.cycleData.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, t.time.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, t.rangeStart.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, t.rangeEnd.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, t.programPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, t.arguments.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 13, t.ringEnabled ? 1 : 0);
    sqlite3_bind_text(stmt, 14, t.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool RunProgramTaskDao::update(const models::RunProgramTask& t) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;

    const char* sql = "UPDATE run_program_tasks SET sync_status=?, last_modified=?, enabled=?, "
        "cycle_mode=?, cycle_data=?, time=?, range_start=?, range_end=?, program_path=?, "
        "arguments=?, ring_enabled=?, label=? WHERE uuid=?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, t.syncStatus);
    sqlite3_bind_text(stmt, 2, t.lastModified.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, t.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 4, t.cycleMode);
    sqlite3_bind_text(stmt, 5, t.cycleData.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, t.time.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, t.rangeStart.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, t.rangeEnd.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, t.programPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, t.arguments.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 11, t.ringEnabled ? 1 : 0);
    sqlite3_bind_text(stmt, 12, t.label.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, t.uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool RunProgramTaskDao::remove(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    sqlite3* db = Database::instance().handle();
    if (!db) return false;
    const char* sql = "DELETE FROM run_program_tasks WHERE uuid=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

models::RunProgramTask RunProgramTaskDao::rowToTask(sqlite3_stmt* stmt) {
    models::RunProgramTask t;
    auto colText = [&](int i) {
        const unsigned char* tx = sqlite3_column_text(stmt, i);
        return tx ? QString::fromUtf8(reinterpret_cast<const char*>(tx)) : QString();
    };
    t.uuid = colText(0);
    t.syncStatus = sqlite3_column_int(stmt, 1);
    t.lastModified = colText(2);
    t.createdAt = colText(3);
    t.enabled = sqlite3_column_int(stmt, 4) != 0;
    t.cycleMode = sqlite3_column_int(stmt, 5);
    t.cycleData = colText(6);
    t.time = colText(7);
    t.rangeStart = colText(8);
    t.rangeEnd = colText(9);
    t.programPath = colText(10);
    t.arguments = colText(11);
    t.ringEnabled = sqlite3_column_int(stmt, 12) != 0;
    t.label = colText(13);
    return t;
}

static const char* RUNPROG_COLS = "uuid, sync_status, last_modified, created_at, enabled, "
    "cycle_mode, cycle_data, time, range_start, range_end, program_path, arguments, ring_enabled, label";

models::RunProgramTask RunProgramTaskDao::findByUuid(const QString& uuid) {
    QMutexLocker lock(&Database::instance().mutex());
    models::RunProgramTask t;
    sqlite3* db = Database::instance().handle();
    if (!db) return t;
    QString sql = QString("SELECT %1 FROM run_program_tasks WHERE uuid=?").arg(RUNPROG_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return t;
    sqlite3_bind_text(stmt, 1, uuid.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) t = rowToTask(stmt);
    sqlite3_finalize(stmt);
    return t;
}

QList<models::RunProgramTask> RunProgramTaskDao::findAll() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::RunProgramTask> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM run_program_tasks ORDER BY created_at DESC").arg(RUNPROG_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToTask(stmt));
    sqlite3_finalize(stmt);
    return list;
}

QList<models::RunProgramTask> RunProgramTaskDao::findUnsynced() {
    QMutexLocker lock(&Database::instance().mutex());
    QList<models::RunProgramTask> list;
    sqlite3* db = Database::instance().handle();
    if (!db) return list;
    QString sql = QString("SELECT %1 FROM run_program_tasks WHERE sync_status != 1").arg(RUNPROG_COLS);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) list.append(rowToTask(stmt));
    sqlite3_finalize(stmt);
    return list;
}

} // namespace mcclock::dal
