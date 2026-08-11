#pragma once

#include <QObject>
#include <QTimer>
#include <QList>
#include "sync_types.h"

namespace mcclock::sync {

// Stub sync manager - collects local changes but does not send them
class SyncManager : public QObject {
    Q_OBJECT
public:
    explicit SyncManager(QObject* parent = nullptr)
        : QObject(parent) {}

    void setApi(std::unique_ptr<ISyncApi> api) {
        api_ = std::move(api);
    }

    void startAutoSync(int intervalMinutes) {
        if (intervalMinutes <= 0) return;
        if (!autoSyncTimer_) {
            autoSyncTimer_ = new QTimer(this);
            connect(autoSyncTimer_, &QTimer::timeout, this, &SyncManager::syncNow);
        }
        autoSyncTimer_->start(intervalMinutes * 60 * 1000);
    }

    void stopAutoSync() {
        if (autoSyncTimer_) autoSyncTimer_->stop();
    }

    void syncNow() {
        // Stub: collect changes but don't send
        emit syncStarted();
        // TODO: Implement actual sync when cloud backend is ready
        emit syncCompleted(false);
    }

signals:
    void syncStarted();
    void syncCompleted(bool success);

private:
    std::unique_ptr<ISyncApi> api_;
    QTimer* autoSyncTimer_ = nullptr;
};

} // namespace mcclock::sync
