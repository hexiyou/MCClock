#pragma once

#include <QList>
#include <QString>
#include <QDateTime>
#include "core/models/all_models.h"

namespace mcclock::services {

// ── Alarm Service: CRUD + recycle bin + missed detection ──
class AlarmService {
public:
    // Adds uuid/timestamps, persists; returns the saved alarm (with uuid)
    models::Alarm add(models::Alarm alarm);
    bool update(models::Alarm alarm);
    bool moveToRecycleBin(const QString& uuid);
    bool restore(const QString& uuid);
    bool hardDelete(const QString& uuid);
    bool clearRecycleBin();
    bool setEnabled(const QString& uuid, bool enabled);

    models::Alarm findByUuid(const QString& uuid);
    QList<models::Alarm> findAll();
    QList<models::Alarm> findByGroup(const QString& groupId);
    QList<models::Alarm> findDeleted();

    // Alarms whose trigger time passed while the app was not running
    QList<models::Alarm> findMissed(const QDateTime& lastRunTime);
};

// ── Birthday Service: CRUD + next occurrence computation ──
class BirthdayService {
public:
    models::Birthday add(models::Birthday b);
    bool update(models::Birthday b);
    bool remove(const QString& uuid);

    models::Birthday findByUuid(const QString& uuid);
    QList<models::Birthday> findAll();

    // Next birthday date (this year or next year), handling lunar conversion
    QDate nextBirthdayDate(const models::Birthday& b);
    // Days remaining until next birthday (0 = today)
    int daysUntilBirthday(const models::Birthday& b);
};

// ── Shutdown Service ──
class ShutdownService {
public:
    models::ShutdownTask add(models::ShutdownTask t);
    bool update(models::ShutdownTask t);
    bool remove(const QString& uuid);
    bool setEnabled(const QString& uuid, bool enabled);

    models::ShutdownTask findByUuid(const QString& uuid);
    QList<models::ShutdownTask> findAll();

    // Execute the shutdown action immediately
    bool executeNow(const models::ShutdownTask& t);
};

// ── Run Program Service ──
class RunProgramService {
public:
    models::RunProgramTask add(models::RunProgramTask t);
    bool update(models::RunProgramTask t);
    bool remove(const QString& uuid);
    bool setEnabled(const QString& uuid, bool enabled);

    models::RunProgramTask findByUuid(const QString& uuid);
    QList<models::RunProgramTask> findAll();

    // Launch the program / open URL immediately
    bool executeNow(const models::RunProgramTask& t);
    static bool isUrl(const QString& path);
};

// ── Countdown Service ──
class CountdownService {
public:
    models::Countdown add(models::Countdown c);
    bool update(models::Countdown c);
    bool remove(const QString& uuid);
    bool setEnabled(const QString& uuid, bool enabled);
    // Persist remaining seconds without touching lastModified semantics
    bool saveRemaining(const QString& uuid, int remainingSeconds);

    models::Countdown findByUuid(const QString& uuid);
    QList<models::Countdown> findAll();
};

// ── Health Reminder Service (single-row settings) ──
class HealthService {
public:
    // Returns current settings, creating a default row if absent
    models::HealthSettings get();
    bool save(models::HealthSettings h);
};

} // namespace mcclock::services
