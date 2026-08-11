#pragma once

#include <QString>
#include <QMutex>

struct sqlite3;
struct sqlite3_stmt;

namespace mcclock::dal {

class Database {
public:
    static Database& instance();

    // Initialize database: open and run migrations
    bool initialize(const QString& dbPath);

    // Close database
    void close();

    // Check if database is open
    bool isOpen() const;

    // Execute SQL (no result)
    bool execute(const QString& sql);

    // Get current schema version
    int schemaVersion() const;

    // Get raw sqlite3 handle (for DAO classes)
    sqlite3* handle();

    // Thread-safe mutex
    QMutex& mutex();

private:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3* db_ = nullptr;
    int schemaVersion_ = 0;
    QMutex mutex_;

    void migrate();
    void migrateToV1();
};

} // namespace mcclock::dal
