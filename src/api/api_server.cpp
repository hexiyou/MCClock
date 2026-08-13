#include "api_server.h"
#include "core/services/business_services.h"
#include "core/services/scheduler.h"
#include "core/dal/settings_manager.h"
#include "core/utils/platform_utils.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QCoreApplication>
#include <QDebug>
#include <functional>
#include <string>
#include <thread>
#include <atomic>

namespace mcclock::api {

using json = nlohmann::json;
using namespace mcclock::services;
using namespace mcclock::models;

namespace {

// ── JSON conversion helpers (QJson <-> nlohmann) ──
json qToJson(const QJsonObject& obj) {
    return json::parse(
        QString(QJsonDocument(obj).toJson(QJsonDocument::Compact)).toStdString());
}

QJsonObject jsonToQ(const json& j) {
    return QJsonDocument::fromJson(QByteArray::fromStdString(j.dump())).object();
}

// ── Standard response envelope: { code, message, data } ──
void respondOk(httplib::Response& res, const json& data = nullptr) {
    json r;
    r["code"] = 0;
    r["message"] = "success";
    r["data"] = data;
    res.set_content(r.dump(), "application/json");
}

void respondFail(httplib::Response& res, const std::string& message, int code = 1) {
    json r;
    r["code"] = code;
    r["message"] = message;
    r["data"] = nullptr;
    res.set_content(r.dump(), "application/json");
}

bool parseBody(const httplib::Request& req, json& out) {
    try {
        out = json::parse(req.body);
        return true;
    } catch (...) {
        return false;
    }
}

// Generic CRUD operations bound to a specific service
struct CrudOps {
    std::function<json()> list;
    std::function<json(const json&)> add;
    std::function<bool(const json&)> update;
    std::function<bool(const QString&)> remove;
};

template <typename Model, typename List>
json listToJson(const List& items) {
    json arr = json::array();
    for (const auto& m : items) arr.push_back(qToJson(m.toJson()));
    return arr;
}

void registerCrud(httplib::Server& svr, const std::string& base, CrudOps ops,
                  const std::function<void()>& onChanged) {
    svr.Get(base, [ops, onChanged](const httplib::Request&, httplib::Response& res) {
        respondOk(res, ops.list());
    });

    svr.Post(base, [ops, onChanged](const httplib::Request& req, httplib::Response& res) {
        json body;
        if (!parseBody(req, body) || !body.is_object()) {
            respondFail(res, "invalid json body");
            return;
        }
        json created = ops.add(body);
        onChanged();
        respondOk(res, created);
    });

    svr.Put(base, [ops, onChanged](const httplib::Request& req, httplib::Response& res) {
        json body;
        if (!parseBody(req, body) || !body.is_object()) {
            respondFail(res, "invalid json body");
            return;
        }
        if (ops.update(body)) {
            onChanged();
            respondOk(res);
        } else {
            respondFail(res, "update failed");
        }
    });

    svr.Delete(base + R"(/([^/]+))",
               [ops, onChanged](const httplib::Request& req, httplib::Response& res) {
        QString uuid = QString::fromStdString(req.matches[1].str());
        if (ops.remove(uuid)) {
            onChanged();
            respondOk(res);
        } else {
            respondFail(res, "delete failed");
        }
    });
}

// Recursively merge a JSON object into settings via the generic set() accessor
void mergeIntoSettings(const QJsonObject& obj, const QStringList& prefix) {
    auto& s = mcclock::dal::SettingsManager::instance();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QStringList path = prefix;
        path << it.key();
        if (it.value().isObject()) {
            mergeIntoSettings(it.value().toObject(), path);
        } else {
            s.set(path, it.value());
        }
    }
}

} // namespace

// ── Worker: owns the httplib server and runs on the dedicated thread ──
class ApiServer::Worker : public QObject {
    Q_OBJECT
public:
    Worker() = default;
    ~Worker() override {
        // Destructor should not call svr_.stop() as it may block
        // The server should be stopped explicitly before destruction
    }

    httplib::Server svr_;
    std::atomic<bool> stopped_{false};
    std::atomic<bool> running_{false};

    void setupRoutes() {
        auto onChanged = [this]() { emit dataChanged(); };

        // Alarms (DELETE moves to recycle bin)
        registerCrud(svr_, "/api/v1/alarms", CrudOps{
            []() { return listToJson<Alarm>(AlarmService().findAll()); },
            [](const json& body) {
                auto saved = AlarmService().add(Alarm::fromJson(jsonToQ(body)));
                return qToJson(saved.toJson());
            },
            [](const json& body) { return AlarmService().update(Alarm::fromJson(jsonToQ(body))); },
            [](const QString& uuid) { return AlarmService().moveToRecycleBin(uuid); }
        }, onChanged);

        // Birthdays
        registerCrud(svr_, "/api/v1/birthdays", CrudOps{
            []() { return listToJson<Birthday>(BirthdayService().findAll()); },
            [](const json& body) {
                auto saved = BirthdayService().add(Birthday::fromJson(jsonToQ(body)));
                return qToJson(saved.toJson());
            },
            [](const json& body) { return BirthdayService().update(Birthday::fromJson(jsonToQ(body))); },
            [](const QString& uuid) { return BirthdayService().remove(uuid); }
        }, onChanged);

        // Shutdown tasks
        registerCrud(svr_, "/api/v1/shutdown-tasks", CrudOps{
            []() { return listToJson<ShutdownTask>(ShutdownService().findAll()); },
            [](const json& body) {
                auto saved = ShutdownService().add(ShutdownTask::fromJson(jsonToQ(body)));
                return qToJson(saved.toJson());
            },
            [](const json& body) { return ShutdownService().update(ShutdownTask::fromJson(jsonToQ(body))); },
            [](const QString& uuid) { return ShutdownService().remove(uuid); }
        }, onChanged);

        // Run program tasks
        registerCrud(svr_, "/api/v1/run-programs", CrudOps{
            []() { return listToJson<RunProgramTask>(RunProgramService().findAll()); },
            [](const json& body) {
                auto saved = RunProgramService().add(RunProgramTask::fromJson(jsonToQ(body)));
                return qToJson(saved.toJson());
            },
            [](const json& body) { return RunProgramService().update(RunProgramTask::fromJson(jsonToQ(body))); },
            [](const QString& uuid) { return RunProgramService().remove(uuid); }
        }, onChanged);

        // Countdowns
        registerCrud(svr_, "/api/v1/countdowns", CrudOps{
            []() { return listToJson<Countdown>(CountdownService().findAll()); },
            [](const json& body) {
                auto saved = CountdownService().add(Countdown::fromJson(jsonToQ(body)));
                return qToJson(saved.toJson());
            },
            [](const json& body) { return CountdownService().update(Countdown::fromJson(jsonToQ(body))); },
            [](const QString& uuid) { return CountdownService().remove(uuid); }
        }, onChanged);

        // Stopwatch control — uses process-wide singleton shared with the GUI page
        auto stopwatchStatus = []() {
            auto& sw = StopwatchService::instance();
            json data;
            data["state"] = sw.state() == StopwatchService::State::Running ? "running"
                          : sw.state() == StopwatchService::State::Paused ? "paused" : "stopped";
            data["elapsedMs"] = sw.elapsedMs();
            json laps = json::array();
            for (auto ms : sw.laps()) laps.push_back(ms);
            data["laps"] = laps;
            return data;
        };
        svr_.Get("/api/v1/stopwatch/status",
                 [stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/start",
                  [stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            StopwatchService::instance().start();
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/pause",
                  [stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            StopwatchService::instance().pause();
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/stop",
                  [stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            StopwatchService::instance().reset();
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/reset",
                  [stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            StopwatchService::instance().reset();
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/lap",
                  [stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            StopwatchService::instance().lap();
            respondOk(res, stopwatchStatus());
        });

        // ── Alarm recycle bin ──
        svr_.Get("/api/v1/alarms/deleted",
                 [onChanged](const httplib::Request&, httplib::Response& res) {
            respondOk(res, listToJson<Alarm>(AlarmService().findDeleted()));
        });
        svr_.Post(R"(/api/v1/alarms/restore/([^/]+))",
                  [onChanged](const httplib::Request& req, httplib::Response& res) {
            QString uuid = QString::fromStdString(req.matches[1].str());
            if (AlarmService().restore(uuid)) { onChanged(); respondOk(res); }
            else respondFail(res, "restore failed");
        });
        svr_.Post(R"(/api/v1/alarms/purge/([^/]+))",
                  [onChanged](const httplib::Request& req, httplib::Response& res) {
            QString uuid = QString::fromStdString(req.matches[1].str());
            if (AlarmService().hardDelete(uuid)) { onChanged(); respondOk(res); }
            else respondFail(res, "purge failed");
        });
        svr_.Post("/api/v1/alarms/clear-recycle",
                  [onChanged](const httplib::Request&, httplib::Response& res) {
            if (AlarmService().clearRecycleBin()) { onChanged(); respondOk(res); }
            else respondFail(res, "clear-recycle failed");
        });

        // ── Alarm groups CRUD ──
        svr_.Get("/api/v1/alarm-groups",
                 [](const httplib::Request&, httplib::Response& res) {
            respondOk(res, listToJson<AlarmGroup>(AlarmGroupService().findAll()));
        });
        svr_.Post("/api/v1/alarm-groups",
                  [onChanged](const httplib::Request& req, httplib::Response& res) {
            auto body = json::parse(req.body, nullptr, false);
            if (body.is_discarded()) { respondFail(res, "invalid JSON"); return; }
            auto saved = AlarmGroupService().add(AlarmGroup::fromJson(jsonToQ(body)));
            if (saved.uuid.isEmpty()) { respondFail(res, "add failed"); return; }
            onChanged();
            respondOk(res, qToJson(saved.toJson()));
        });
        svr_.Put(R"(/api/v1/alarm-groups/([^/]+))",
                 [onChanged](const httplib::Request& req, httplib::Response& res) {
            QString uuid = QString::fromStdString(req.matches[1].str());
            auto body = json::parse(req.body, nullptr, false);
            if (body.is_discarded()) { respondFail(res, "invalid JSON"); return; }
            auto group = AlarmGroup::fromJson(jsonToQ(body));
            group.uuid = uuid;
            if (AlarmGroupService().update(group)) { onChanged(); respondOk(res); }
            else respondFail(res, "update failed");
        });
        svr_.Delete(R"(/api/v1/alarm-groups/([^/]+))",
                    [onChanged](const httplib::Request& req, httplib::Response& res) {
            QString uuid = QString::fromStdString(req.matches[1].str());
            if (AlarmGroupService().remove(uuid)) { onChanged(); respondOk(res); }
            else respondFail(res, "delete failed");
        });

        // ── Execute now ──
        svr_.Post(R"(/api/v1/run-programs/([^/]+)/run)",
                  [onChanged](const httplib::Request& req, httplib::Response& res) {
            QString uuid = QString::fromStdString(req.matches[1].str());
            RunProgramTask t = RunProgramService().findByUuid(uuid);
            if (t.uuid.isEmpty()) { respondFail(res, "not found"); return; }
            if (RunProgramService().executeNow(t)) { onChanged(); respondOk(res); }
            else respondFail(res, "launch failed");
        });
        svr_.Post(R"(/api/v1/shutdown-tasks/([^/]+)/run)",
                  [onChanged](const httplib::Request& req, httplib::Response& res) {
            QString uuid = QString::fromStdString(req.matches[1].str());
            ShutdownTask t = ShutdownService().findByUuid(uuid);
            if (t.uuid.isEmpty()) { respondFail(res, "not found"); return; }
            if (ShutdownService().executeNow(t)) { onChanged(); respondOk(res); }
            else respondFail(res, "execute failed");
        });

        // ── Health settings ──
        svr_.Get("/api/v1/health", [](const httplib::Request&, httplib::Response& res) {
            respondOk(res, qToJson(HealthService().get().toJson()));
        });
        svr_.Put("/api/v1/health", [onChanged](const httplib::Request& req, httplib::Response& res) {
            json body;
            if (!parseBody(req, body) || !body.is_object()) {
                respondFail(res, "invalid json body");
                return;
            }
            auto h = HealthService().get();
            QJsonObject jo = jsonToQ(body);
            if (jo.contains("enabled"))    h.enabled = jo["enabled"].toBool();
            if (jo.contains("displayMode")) h.displayMode = jo["displayMode"].toInt();
            if (jo.contains("workMinutes")) h.workMinutes = jo["workMinutes"].toInt();
            if (jo.contains("restMinutes")) h.restMinutes = jo["restMinutes"].toInt();
            if (jo.contains("ringtone"))    h.ringtone = jo["ringtone"].toInt();
            if (jo.contains("ringMode"))    h.ringMode = jo["ringMode"].toInt();
            if (jo.contains("customRingtonePath")) h.customRingtonePath = jo["customRingtonePath"].toString();
            if (jo.contains("customMinutes")) h.customMinutes = jo["customMinutes"].toInt();
            if (jo.contains("label"))       h.label = jo["label"].toString();
            HealthService().save(h);
            onChanged();
            respondOk(res, qToJson(h.toJson()));
        });

        // Settings
        svr_.Get("/api/v1/settings", [](const httplib::Request&, httplib::Response& res) {
            respondOk(res, qToJson(mcclock::dal::SettingsManager::instance().rootObject()));
        });
        svr_.Put("/api/v1/settings", [onChanged](const httplib::Request& req, httplib::Response& res) {
            json body;
            if (!parseBody(req, body) || !body.is_object()) {
                respondFail(res, "invalid json body");
                return;
            }
            mergeIntoSettings(jsonToQ(body), {});
            mcclock::dal::SettingsManager::instance().save();
            onChanged();
            respondOk(res);
        });

        // Status
        svr_.Get("/api/v1/status", [](const httplib::Request&, httplib::Response& res) {
            json data;
            data["app"] = "MCClock";
            data["version"] = QCoreApplication::applicationVersion().toStdString();
            data["time"] = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
            data["uptime"] = mcclock::utils::PlatformUtils::uptimeString().toStdString();
            respondOk(res, data);
        });
    }

public slots:
    void run(const QString& bindIp, int port) {
        setupRoutes();
        if (!svr_.bind_to_port(bindIp.toStdString(), port)) {
            qWarning() << "API server bind failed:" << bindIp << port;
            emit failed(QStringLiteral("bind %1:%2 failed").arg(bindIp).arg(port));
            return;
        }
        running_ = true;
        emit started(QStringLiteral("%1:%2").arg(bindIp).arg(port));
        svr_.listen_after_bind();
        running_ = false;
        if (!stopped_) {
            emit stopped();
        }
    }

    void stopServer() {
        stopped_ = true;
        // Use a separate thread to call stop() to avoid blocking
        std::thread stopThread([this]() {
            svr_.stop();
        });
        // Wait briefly for stop to complete, but don't block forever
        if (stopThread.joinable()) {
            stopThread.detach();
        }
    }

signals:
    void started(const QString& address);
    void stopped();
    void failed(const QString& error);
    void dataChanged();
};

// ── ApiServer ──

ApiServer::ApiServer(QObject* parent)
    : QObject(parent)
{
}

ApiServer::~ApiServer() {
    stop();
}

void ApiServer::start(const QString& bindIp, int port) {
    if (running_) return;

    bindIp_ = bindIp;
    port_ = port;

    thread_ = new QThread(this);
    worker_ = new Worker(); // no parent: moved to thread
    worker_->moveToThread(thread_);

    connect(thread_, &QThread::started, worker_, [this, bindIp, port]() {
        worker_->run(bindIp, port);
    });
    connect(worker_, &Worker::started, this, [this](const QString& addr) {
        running_ = true;
        emit serverStarted(addr);
    });
    connect(worker_, &Worker::failed, this, [this](const QString& err) {
        running_ = false;
        emit errorOccurred(err);
    });
    connect(worker_, &Worker::stopped, this, [this]() {
        running_ = false;
        thread_->quit();
        emit serverStopped();
    });
    connect(worker_, &Worker::dataChanged, this, &ApiServer::dataChanged);
    connect(thread_, &QThread::finished, worker_, &QObject::deleteLater);

    thread_->start();
}

void ApiServer::stop() {
    if (!thread_) return;

    // Mark worker as stopping and invoke stopServer
    if (worker_) {
        worker_->stopped_ = true;
        QMetaObject::invokeMethod(worker_, "stopServer", Qt::QueuedConnection);
    }

    // Wait for thread to finish with timeout
    if (thread_->isRunning()) {
        // Try to quit the event loop first
        thread_->quit();

        if (!thread_->wait(3000)) {
            // Force terminate if thread doesn't stop
            qWarning() << "API server thread did not stop gracefully, terminating...";
            thread_->terminate();
            thread_->wait(1000);
        }
    }

    // Cleanup - disconnect signals first to avoid dangling connections
    if (worker_) {
        disconnect(worker_, nullptr, this, nullptr);
    }
    if (thread_) {
        disconnect(thread_, nullptr, nullptr, nullptr);
        thread_->deleteLater();
    }

    thread_ = nullptr;
    worker_ = nullptr;
    running_ = false;
}

bool ApiServer::isRunning() const {
    return running_;
}

QString ApiServer::address() const {
    if (!running_) return QString();
    return bindIp_ + ":" + QString::number(port_);
}

} // namespace mcclock::api

#include "api_server.moc"
