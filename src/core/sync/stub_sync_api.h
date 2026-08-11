#pragma once

#include "sync_types.h"

namespace mcclock::sync {

// Stub implementation of ISyncApi: cloud sync is not available yet.
// Every network operation reports failure with a clear message so that
// the rest of the sync pipeline can be developed and tested offline.
class StubSyncApi : public ISyncApi {
public:
    bool login(const QString& username, const QString& password) override {
        Q_UNUSED(username);
        Q_UNUSED(password);
        // Cloud sync is not available yet
        return false;
    }

    bool registerAccount(const QString& username, const QString& password,
                         const QString& email) override {
        Q_UNUSED(username);
        Q_UNUSED(password);
        Q_UNUSED(email);
        return false;
    }

    bool refreshToken() override { return false; }

    void logout() override {
        token_.clear();
        authenticated_ = false;
    }

    SyncResponse fullSync(const SyncPayload& payload) override {
        Q_UNUSED(payload);
        return notAvailable();
    }

    SyncResponse incrementalSync(const SyncPayload& delta) override {
        Q_UNUSED(delta);
        return notAvailable();
    }

    bool isAuthenticated() const override { return authenticated_; }
    QString authToken() const override { return token_; }

private:
    static SyncResponse notAvailable() {
        SyncResponse r;
        r.success = false;
        // 云同步服务暂未上线
        r.errorMessage = QStringLiteral("cloud sync service is not available yet");
        return r;
    }

    bool authenticated_ = false;
    QString token_;
};

} // namespace mcclock::sync
