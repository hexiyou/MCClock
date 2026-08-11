#pragma once

#include <QObject>
#include <QThread>
#include <QString>

namespace mcclock::api {

// HTTP API Server - runs in a separate thread
// Disabled by default; configurable IP and port in global settings
class ApiServer : public QObject {
    Q_OBJECT
public:
    explicit ApiServer(QObject* parent = nullptr);
    ~ApiServer();

    // Start the API server on the given IP and port
    void start(const QString& bindIp, int port);

    // Stop the API server
    void stop();

    // Check if server is running
    bool isRunning() const;

    // Get the bound address
    QString address() const;

signals:
    void serverStarted(const QString& address);
    void serverStopped();
    void errorOccurred(const QString& error);

private:
    QThread* thread_ = nullptr;
    bool running_ = false;
    QString bindIp_;
    int port_ = 0;
};

} // namespace mcclock::api
