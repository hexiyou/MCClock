#pragma once

#include <QList>
#include <QString>
#include "core/models/all_models.h"

struct sqlite3_stmt;

namespace mcclock::dal {

class AlarmDao {
public:
    bool insert(const models::Alarm& alarm);
    bool update(const models::Alarm& alarm);

    // Soft delete (move to recycle bin)
    bool remove(const QString& uuid);
    // Permanent delete
    bool hardDelete(const QString& uuid);
    // Restore from recycle bin
    bool restore(const QString& uuid);

    models::Alarm findByUuid(const QString& uuid);
    QList<models::Alarm> findAll(bool includeDeleted = false);
    QList<models::Alarm> findByGroup(const QString& groupId);
    QList<models::Alarm> findUnsynced();

    // Recycle bin operations
    bool clearRecycleBin();
    QList<models::Alarm> findDeleted();

private:
    models::Alarm rowToAlarm(sqlite3_stmt* stmt);
};

class AlarmGroupDao {
public:
    bool insert(const models::AlarmGroup& group);
    bool update(const models::AlarmGroup& group);
    bool remove(const QString& uuid);
    models::AlarmGroup findByUuid(const QString& uuid);
    QList<models::AlarmGroup> findAll();
private:
    models::AlarmGroup rowToGroup(sqlite3_stmt* stmt);
};

} // namespace mcclock::dal
