#pragma once

#include <QList>
#include <QString>
#include "core/models/all_models.h"

namespace mcclock::dal {

class ShutdownTaskDao {
public:
    bool insert(const models::ShutdownTask& t);
    bool update(const models::ShutdownTask& t);
    bool remove(const QString& uuid);
    models::ShutdownTask findByUuid(const QString& uuid);
    QList<models::ShutdownTask> findAll();
    QList<models::ShutdownTask> findUnsynced();
private:
    models::ShutdownTask rowToTask(struct sqlite3_stmt* stmt);
};

class RunProgramTaskDao {
public:
    bool insert(const models::RunProgramTask& t);
    bool update(const models::RunProgramTask& t);
    bool remove(const QString& uuid);
    models::RunProgramTask findByUuid(const QString& uuid);
    QList<models::RunProgramTask> findAll();
    QList<models::RunProgramTask> findUnsynced();
private:
    models::RunProgramTask rowToTask(struct sqlite3_stmt* stmt);
};

} // namespace mcclock::dal
