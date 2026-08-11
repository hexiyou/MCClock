#pragma once

#include <QString>
#include <QList>
#include "core/models/all_models.h"

namespace mcclock::sync {

// ── Sync payload: data sent to server ──
struct SyncPayload {
    QString clientId;           // Device unique identifier
    QString timestamp;          // ISO8601
    int schemaVersion = 1;

    QList<models::Alarm> alarms;
    QList<models::AlarmGroup> groups;
    QList<models::Birthday> birthdays;
    QList<models::ShutdownTask> shutdownTasks;
    QList<models::RunProgramTask> runProgramTasks;
    QList<models::Countdown> countdowns;
    QList<models::HealthSettings> healthSettings;

    QJsonObject toJson() const;
    static SyncPayload fromJson(const QJsonObject& json);
};

// ── Sync response from server ──
struct SyncResponse {
    bool success = false;
    QString serverTimestamp;
    QString errorMessage;

    // Server-side newer versions (conflict data)
    QList<models::Alarm> conflictAlarms;
    QList<models::Birthday> conflictBirthdays;
    QList<models::ShutdownTask> conflictShutdownTasks;
    QList<models::RunProgramTask> conflictRunProgramTasks;
    QList<models::Countdown> conflictCountdowns;

    QJsonObject toJson() const;
    static SyncResponse fromJson(const QJsonObject& json);
};

// ── Conflict resolution strategy ──
enum class ConflictResolution {
    ServerWins,
    ClientWins,
    ManualMerge
};

// ── Abstract sync API interface ──
class ISyncApi {
public:
    virtual ~ISyncApi() = default;

    // Authentication
    virtual bool login(const QString& username, const QString& password) = 0;
    virtual bool registerAccount(const QString& username, const QString& password, const QString& email) = 0;
    virtual bool refreshToken() = 0;
    virtual void logout() = 0;

    // Sync operations
    virtual SyncResponse fullSync(const SyncPayload& payload) = 0;
    virtual SyncResponse incrementalSync(const SyncPayload& delta) = 0;

    // Status
    virtual bool isAuthenticated() const = 0;
    virtual QString authToken() const = 0;
};

/*
 * HTTP Request Format Design (for future implementation):
 *
 * POST /api/sync/v1/full
 * Headers:
 *   Authorization: Bearer <token>
 *   Content-Type: application/json
 *   X-Client-ID: <device-uuid>
 *   X-Schema-Version: 1
 * Body: { SyncPayload JSON }
 *
 * POST /api/sync/v1/incremental
 * Headers: same as above
 * Body: { Only records with sync_status != 1 }
 *
 * POST /api/auth/login
 * Body: { "username": "...", "password": "..." }
 * Response: { "token": "...", "expires_in": 86400 }
 *
 * POST /api/auth/register
 * Body: { "username": "...", "password": "...", "email": "..." }
 * Response: { "user_id": "...", "token": "..." }
 */

} // namespace mcclock::sync
