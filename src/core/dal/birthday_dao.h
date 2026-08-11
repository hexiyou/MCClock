#pragma once

#include <QList>
#include <QString>
#include "core/models/all_models.h"

struct sqlite3_stmt;

namespace mcclock::dal {

class BirthdayDao {
public:
    bool insert(const models::Birthday& b);
    bool update(const models::Birthday& b);
    bool remove(const QString& uuid);
    models::Birthday findByUuid(const QString& uuid);
    QList<models::Birthday> findAll();
    QList<models::Birthday> findUnsynced();
private:
    models::Birthday rowToBirthday(sqlite3_stmt* stmt);
};

} // namespace mcclock::dal
