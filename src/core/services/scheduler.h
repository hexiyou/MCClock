#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QSet>
#include "core/models/all_models.h"

namespace mcclock::services {

// Central scheduler: ticks every second, evaluates all timed tasks
// and emits signals when they trigger. Used by the GUI process.
class Scheduler : public QObject {
    Q_OBJECT
public:
    explicit Scheduler(QObject* parent = nullptr);
    ~Scheduler() override;

    void start();
    void stop();
    bool isRunning() const;

    // Reload task caches from the database (call after data changes)
    void reload();

signals:
    void alarmTriggered(const mcclock::models::Alarm& alarm);
    void birthdayTriggered(const mcclock::models::Birthday& birthday);
    // secondsLeft counts down from advance_seconds to 0
    void shutdownWarning(const mcclock::models::ShutdownTask& task, int secondsLeft);
    void shutdownDue(const mcclock::models::ShutdownTask& task);
    void runProgramTriggered(const mcclock::models::RunProgramTask& task);
    void hourlyChime(int hour);
    void tick(const QDateTime& now);

private slots:
    void onTick();

private:
    void evaluateMinuteBoundary(const QDateTime& now);
    void evaluateShutdownWarnings(const QDateTime& now);
    QDateTime triggerDateTime(const models::ShutdownTask& t, const QDateTime& now) const;

    QTimer* timer_ = nullptr;
    int lastMinute_ = -1;
    int lastHour_ = -1;
    QSet<QString> firedKeys_;   // dedupe: uuid@yyyyMMddHHmm
};

// Stopwatch with millisecond precision
class StopwatchService : public QObject {
    Q_OBJECT
public:
    enum class State { Stopped, Running, Paused };

    explicit StopwatchService(QObject* parent = nullptr);

    State state() const { return state_; }
    // Elapsed milliseconds (excluding current run segment if paused/stopped)
    qint64 elapsedMs() const;

    void start();           // Start or resume
    void pause();
    void reset();
    QList<qint64> laps() const { return laps_; }
    qint64 lap();           // Record a lap, returns total elapsed ms

signals:
    void started();
    void paused();
    void resetDone();
    void lapped(qint64 totalMs);

private:
    State state_ = State::Stopped;
    qint64 accumulatedMs_ = 0;   // Time accumulated before current run segment
    QDateTime segmentStart_;     // Start of current running segment
    QList<qint64> laps_;
};

} // namespace mcclock::services
