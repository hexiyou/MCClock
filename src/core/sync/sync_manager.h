#pragma once

#include <QObject>
#include <QTimer>
#include <QList>
#include <memory>
#include "sync_types.h"

namespace mcclock::sync {

// Stub sync manager - collects local changes and applies server changes,
// but the network transport is not wired to a real backend yet.
class SyncManager : public QObject {
    Q_OBJECT
public:
    explicit SyncManager(QObject* parent = nullptr);

    void setApi(std::unique_ptr<ISyncApi> api);

    void startAutoSync(int intervalMinutes);
    void stopAutoSync();

    // Collect all locally unsynced records into a payload
    SyncPayload collectLocalChanges() const;

    // Apply server-side conflict records to the local database
    // (server wins by default in the stub implementation)
    void applyServerChanges(const SyncResponse& response);

public slots:
    void syncNow();

signals:
    void syncStarted();
    void syncCompleted(bool success);

private:
    std::unique_ptr<ISyncApi> api_;
    QTimer* autoSyncTimer_ = nullptr;
};

} // namespace mcclock::sync
