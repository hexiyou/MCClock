#include "api_server.h"
#include "core/services/business_services.h"
#include "core/services/scheduler.h"
#include "core/dal/settings_manager.h"

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
    ~Worker() override { svr_.stop(); }

    httplib::Server svr_;
    StopwatchService stopwatch_;

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

        // Stopwatch control
        auto stopwatchStatus = [this]() {
            json data;
            data["state"] = stopwatch_.state() == StopwatchService::State::Running ? "running"
                          : stopwatch_.state() == StopwatchService::State::Paused ? "paused" : "stopped";
            data["elapsedMs"] = stopwatch_.elapsedMs();
            json laps = json::array();
            for (auto ms : stopwatch_.laps()) laps.push_back(ms);
            data["laps"] = laps;
            return data;
        };
        svr_.Get("/api/v1/stopwatch/status",
                 [stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/start",
                  [this, stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            stopwatch_.start();
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/pause",
                  [this, stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            stopwatch_.pause();
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/stop",
                  [this, stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            stopwatch_.reset();
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/reset",
                  [this, stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            stopwatch_.reset();
            respondOk(res, stopwatchStatus());
        });
        svr_.Post("/api/v1/stopwatch/lap",
                  [this, stopwatchStatus](const httplib::Request&, httplib::Response& res) {
            stopwatch_.lap();
            respondOk(res, stopwatchStatus());
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
        emit started(QStringLiteral("%1:%2").arg(bindIp).arg(port));
        svr_.listen_after_bind();
        emit stopped();
    }

    void stopServer() {
        svr_.stop();
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
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "stopServer", Qt::QueuedConnection);
    }
    if (!thread_->wait(3000)) {
        thread_->terminate();
        thread_->wait(1000);
    }
    thread_->deleteLater();
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
