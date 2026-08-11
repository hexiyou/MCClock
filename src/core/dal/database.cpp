#include "database.h"
#include "sqlite3.h"

#include <QDebug>

namespace mcclock::dal {

Database& Database::instance() {
    static Database db;
    return db;
}

Database::~Database() {
    close();
}

bool Database::initialize(const QString& dbPath) {
    QMutexLocker lock(&mutex_);

    if (db_) {
        return true; // Already initialized
    }

    QByteArray pathBytes = dbPath.toUtf8();
    int rc = sqlite3_open(pathBytes.constData(), &db_);
    if (rc != SQLITE_OK) {
        qCritical() << "Failed to open database:" << sqlite3_errmsg(db_);
        db_ = nullptr;
        return false;
    }

    // Enable WAL mode for better concurrent access (GUI + CLI)
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

    // Get current schema version
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db_,
        "SELECT version FROM schema_version ORDER BY version DESC LIMIT 1",
        -1, &stmt, nullptr);

    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        schemaVersion_ = sqlite3_column_int(stmt, 0);
    }
    if (stmt) sqlite3_finalize(stmt);

    // If schema_version table doesn't exist, create it
    if (schemaVersion_ == 0) {
        sqlite3_exec(db_,
            "CREATE TABLE IF NOT EXISTS schema_version ("
            "  version INTEGER PRIMARY KEY,"
            "  applied_at TEXT DEFAULT (datetime('now'))"
            ");",
            nullptr, nullptr, nullptr);
    }

    migrate();

    qDebug() << "Database initialized at:" << dbPath
             << "Schema version:" << schemaVersion_;
    return true;
}

void Database::close() {
    QMutexLocker lock(&mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        schemaVersion_ = 0;
    }
}

bool Database::isOpen() const {
    return db_ != nullptr;
}

bool Database::execute(const QString& sql) {
    QMutexLocker lock(&mutex_);
    if (!db_) return false;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.toUtf8().constData(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qWarning() << "SQL error:" << (errMsg ? errMsg : "unknown")
                   << "\nSQL:" << sql;
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}

int Database::schemaVersion() const {
    return schemaVersion_;
}

sqlite3* Database::handle() {
    return db_;
}

QMutex& Database::mutex() {
    return mutex_;
}

void Database::migrate() {
    if (schemaVersion_ < 1) {
        migrateToV1();
    }
    // Future migrations:
    // if (schemaVersion_ < 2) { migrateToV2(); }
}

void Database::migrateToV1() {
    const char* sql = R"SQL(
        -- Alarm groups
        CREATE TABLE IF NOT EXISTS alarm_groups (
            uuid TEXT PRIMARY KEY,
            sync_status INTEGER DEFAULT 0,
            last_modified TEXT,
            created_at TEXT,
            name TEXT NOT NULL,
            sort_order INTEGER DEFAULT 0
        );

        -- Insert default group if not exists
        INSERT OR IGNORE INTO alarm_groups (uuid, sync_status, last_modified, created_at, name, sort_order)
        VALUES ('default', 1, datetime('now'), datetime('now'), '默认', 0);

        -- Alarms
        CREATE TABLE IF NOT EXISTS alarms (
            uuid TEXT PRIMARY KEY,
            sync_status INTEGER DEFAULT 0,
            last_modified TEXT,
            created_at TEXT,
            enabled INTEGER DEFAULT 1,
            cycle_mode INTEGER DEFAULT 0,
            cycle_data TEXT DEFAULT '{}',
            time TEXT NOT NULL,
            range_start TEXT,
            range_end TEXT,
            ringtone INTEGER DEFAULT 1,
            custom_ringtone_path TEXT DEFAULT '',
            ring_mode INTEGER DEFAULT 0,
            custom_minutes INTEGER DEFAULT 0,
            label TEXT DEFAULT '',
            group_id TEXT DEFAULT 'default',
            deleted INTEGER DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_alarms_deleted ON alarms(deleted);
        CREATE INDEX IF NOT EXISTS idx_alarms_enabled ON alarms(enabled);
        CREATE INDEX IF NOT EXISTS idx_alarms_sync ON alarms(sync_status);

        -- Birthdays
        CREATE TABLE IF NOT EXISTS birthdays (
            uuid TEXT PRIMARY KEY,
            sync_status INTEGER DEFAULT 0,
            last_modified TEXT,
            created_at TEXT,
            name TEXT NOT NULL,
            gender INTEGER DEFAULT 0,
            is_lunar INTEGER DEFAULT 0,
            solar_year INTEGER,
            solar_month INTEGER,
            solar_day INTEGER,
            lunar_month INTEGER,
            lunar_day INTEGER,
            remind_time TEXT DEFAULT '08:00',
            advance_days INTEGER DEFAULT 1,
            ringtone INTEGER DEFAULT 1,
            custom_ringtone_path TEXT DEFAULT '',
            ring_mode INTEGER DEFAULT 0,
            custom_minutes INTEGER DEFAULT 0,
            avatar_path TEXT DEFAULT '',
            label TEXT DEFAULT ''
        );
        CREATE INDEX IF NOT EXISTS idx_birthdays_sync ON birthdays(sync_status);

        -- Shutdown tasks
        CREATE TABLE IF NOT EXISTS shutdown_tasks (
            uuid TEXT PRIMARY KEY,
            sync_status INTEGER DEFAULT 0,
            last_modified TEXT,
            created_at TEXT,
            enabled INTEGER DEFAULT 1,
            cycle_mode INTEGER DEFAULT 0,
            cycle_data TEXT DEFAULT '{}',
            time TEXT NOT NULL,
            range_start TEXT,
            range_end TEXT,
            shutdown_option INTEGER DEFAULT 1,
            advance_seconds INTEGER DEFAULT 30,
            label TEXT DEFAULT ''
        );
        CREATE INDEX IF NOT EXISTS idx_shutdown_sync ON shutdown_tasks(sync_status);

        -- Run program tasks
        CREATE TABLE IF NOT EXISTS run_program_tasks (
            uuid TEXT PRIMARY KEY,
            sync_status INTEGER DEFAULT 0,
            last_modified TEXT,
            created_at TEXT,
            enabled INTEGER DEFAULT 1,
            cycle_mode INTEGER DEFAULT 0,
            cycle_data TEXT DEFAULT '{}',
            time TEXT NOT NULL,
            range_start TEXT,
            range_end TEXT,
            program_path TEXT NOT NULL,
            arguments TEXT DEFAULT '',
            ring_enabled INTEGER DEFAULT 0,
            label TEXT DEFAULT ''
        );
        CREATE INDEX IF NOT EXISTS idx_runprog_sync ON run_program_tasks(sync_status);

        -- Countdowns
        CREATE TABLE IF NOT EXISTS countdowns (
            uuid TEXT PRIMARY KEY,
            sync_status INTEGER DEFAULT 0,
            last_modified TEXT,
            created_at TEXT,
            enabled INTEGER DEFAULT 1,
            mode INTEGER DEFAULT 0,
            total_seconds INTEGER DEFAULT 0,
            target_datetime TEXT,
            remaining_seconds INTEGER DEFAULT 0,
            ringtone INTEGER DEFAULT 1,
            custom_ringtone_path TEXT DEFAULT '',
            ring_mode INTEGER DEFAULT 1,
            custom_minutes INTEGER DEFAULT 0,
            label TEXT DEFAULT ''
        );
        CREATE INDEX IF NOT EXISTS idx_countdowns_sync ON countdowns(sync_status);

        -- Health settings (single row typically)
        CREATE TABLE IF NOT EXISTS health_settings (
            uuid TEXT PRIMARY KEY,
            sync_status INTEGER DEFAULT 0,
            last_modified TEXT,
            created_at TEXT,
            enabled INTEGER DEFAULT 0,
            display_mode INTEGER DEFAULT 1,
            work_minutes INTEGER DEFAULT 45,
            rest_minutes INTEGER DEFAULT 5,
            ringtone INTEGER DEFAULT 1,
            custom_ringtone_path TEXT DEFAULT '',
            ring_mode INTEGER DEFAULT 0,
            custom_minutes INTEGER DEFAULT 0,
            label TEXT DEFAULT ''
        );

        -- Record migration
        INSERT INTO schema_version (version) VALUES (1);
    )SQL";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        qCritical() << "Migration V1 failed:" << (errMsg ? errMsg : "unknown");
        if (errMsg) sqlite3_free(errMsg);
        return;
    }

    schemaVersion_ = 1;
    qDebug() << "Database migrated to schema version 1";
}

} // namespace mcclock::dal
