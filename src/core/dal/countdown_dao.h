#pragma once

#include <QList>
#include <QString>
#include "core/models/all_models.h"

struct sqlite3_stmt;

namespace mcclock::dal {

class CountdownDao {
public:
    bool insert(const models::Countdown& c);
    bool update(const models::Countdown& c);
    bool remove(const QString& uuid);
    models::Countdown findByUuid(const QString& uuid);
    QList<models::Countdown> findAll();
    QList<models::Countdown> findUnsynced();
private:
    models::Countdown rowToCountdown(sqlite3_stmt* stmt);
};

class HealthSettingsDao {
public:
    bool upsert(const models::HealthSettings& h);
    bool remove(const QString& uuid);
    models::HealthSettings findActive();
    QList<models::HealthSettings> findAll();
    QList<models::HealthSettings> findUnsynced();
private:
    models::HealthSettings rowToSettings(sqlite3_stmt* stmt);
};

} // namespace mcclock::dal
