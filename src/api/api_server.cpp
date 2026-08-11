#include "api_server.h"
#include <QDebug>

namespace mcclock::api {

ApiServer::ApiServer(QObject* parent)
    : QObject(parent)
{
}

ApiServer::~ApiServer() {
    stop();
}

void ApiServer::start(const QString& bindIp, int port) {
    if (running_) {
        qWarning() << "API server already running";
        return;
    }

    bindIp_ = bindIp;
    port_ = port;

    // TODO: Create cpp-httplib server in a separate thread
    // and register routes for all API endpoints
    //
    // Routes to implement (P6):
    //   GET/POST/PUT/DELETE /api/v1/alarms
    //   GET/POST/PUT/DELETE /api/v1/birthdays
    //   GET/POST/PUT/DELETE /api/v1/shutdown-tasks
    //   GET/POST/PUT/DELETE /api/v1/run-programs
    //   GET/POST/PUT/DELETE /api/v1/countdowns
    //   GET /api/v1/stopwatch/status
    //   POST /api/v1/stopwatch/start|pause|stop|lap|reset
    //   GET/PUT /api/v1/settings
    //   GET /api/v1/status

    running_ = true;
    QString addr = bindIp + ":" + QString::number(port);
    qDebug() << "API server started at" << addr;
    emit serverStarted(addr);
}

void ApiServer::stop() {
    if (!running_) return;

    // TODO: Stop the httplib server

    running_ = false;
    qDebug() << "API server stopped";
    emit serverStopped();
}

bool ApiServer::isRunning() const {
    return running_;
}

QString ApiServer::address() const {
    if (!running_) return QString();
    return bindIp_ + ":" + QString::number(port_);
}

} // namespace mcclock::api
