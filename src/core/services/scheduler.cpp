#include "scheduler.h"
#include "cycle_utils.h"
#include "business_services.h"
#include "core/dal/alarm_dao.h"
#include "core/dal/birthday_dao.h"
#include "core/dal/task_dao.h"
#include "core/dal/settings_manager.h"

#include <QTime>
#include <QDate>
#include <QJsonArray>

namespace mcclock::services {

// ── Scheduler ──

Scheduler::Scheduler(QObject* parent)
    : QObject(parent)
{
    timer_ = new QTimer(this);
    timer_->setInterval(1000);
    connect(timer_, &QTimer::timeout, this, &Scheduler::onTick);
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (!timer_->isActive()) {
        lastMinute_ = -1;
        lastHour_ = -1;
        firedKeys_.clear();
        timer_->start();
    }
}

void Scheduler::stop() {
    timer_->stop();
}

bool Scheduler::isRunning() const {
    return timer_->isActive();
}

void Scheduler::reload() {
    firedKeys_.clear();
}

void Scheduler::onTick() {
    QDateTime now = QDateTime::currentDateTime();
    emit tick(now);

    evaluateShutdownWarnings(now);
    evaluateIntervalTasks(now);

    int minute = now.time().minute();
    if (minute != lastMinute_) {
        lastMinute_ = minute;
        evaluateMinuteBoundary(now);
    }
}

void Scheduler::evaluateMinuteBoundary(const QDateTime& now) {
    QDate today = now.date();
    QString hhmm = now.time().toString("HH:mm");
    QString dedupeSuffix = now.toString("yyyyMMddHHmm");
    int minute = now.time().minute();
    int hour = now.time().hour();

    // Hourly chime (based on settings: mode/cycle/hours).
    // Not fired on the very first tick after app start.
    if (lastMinute_ != minute || lastHour_ != -1) {
        auto& settings = dal::SettingsManager::instance();
        QString mode = settings.chimeMode();
        if (mode != "off" && !mode.isEmpty()) {
            QString cycle = settings.chimeCycle();
            bool shouldChime = false;
            if (cycle == "hourly" && minute == 0) {
                shouldChime = true;
            } else if (cycle == "half_hour" && (minute == 0 || minute == 30)) {
                shouldChime = true;
            } else if (cycle == "custom") {
                const QJsonArray hours = settings.chimeHours();
                int interval = settings.chimeMinute();
                if (interval > 0 && minute % interval == 0) {
                    for (const auto& h : hours) {
                        if (h.toInt() == hour) { shouldChime = true; break; }
                    }
                }
            }
            if (shouldChime) {
                QString key = QString("chime@%1").arg(dedupeSuffix);
                if (!firedKeys_.contains(key)) {
                    firedKeys_.insert(key);
                    emit hourlyChime(hour, minute);
                }
            }
        }
    }
    lastHour_ = hour;

    // Alarms
    const auto alarms = dal::AlarmDao().findAll();
    for (const auto& a : alarms) {
        if (!a.enabled || a.deleted) continue;
        if (a.time != hhmm) continue;
        if (!CycleUtils::occursOnDate(today, a.cycleMode, a.cycleData)) continue;
        QString key = a.uuid + "@" + dedupeSuffix;
        if (firedKeys_.contains(key)) continue;
        firedKeys_.insert(key);
        emit alarmTriggered(a);
    }

    // Birthdays: trigger at remindTime on the birthday itself and on
    // advance-days before it
    const auto birthdays = dal::BirthdayDao().findAll();
    BirthdayService bs;
    for (const auto& b : birthdays) {
        if (b.remindTime != hhmm) continue;
        QDate next = bs.nextBirthdayDate(b);
        if (!next.isValid()) continue;
        int daysUntil = today.daysTo(next);
        if (daysUntil < 0 || daysUntil > b.advanceDays) continue;
        QString key = b.uuid + "@" + dedupeSuffix;
        if (firedKeys_.contains(key)) continue;
        firedKeys_.insert(key);
        emit birthdayTriggered(b);
    }

    // Run program tasks (skip interval mode — handled by evaluateIntervalTasks)
    const auto progs = dal::RunProgramTaskDao().findAll();
    for (const auto& t : progs) {
        if (!t.enabled) continue;
        if (t.cycleMode == 5) continue; // interval mode managed separately
        if (t.time != hhmm) continue;
        if (!CycleUtils::occursOnDate(today, t.cycleMode, t.cycleData)) continue;
        QString key = t.uuid + "@" + dedupeSuffix;
        if (firedKeys_.contains(key)) continue;
        firedKeys_.insert(key);
        emit runProgramTriggered(t);
    }

    // Shutdown due (in case warning window was missed)
    const auto tasks = dal::ShutdownTaskDao().findAll();
    for (const auto& t : tasks) {
        if (!t.enabled) continue;
        if (t.time != hhmm) continue;
        if (!CycleUtils::occursOnDate(today, t.cycleMode, t.cycleData)) continue;
        QString key = t.uuid + "@due@" + dedupeSuffix;
        if (firedKeys_.contains(key)) continue;
        firedKeys_.insert(key);
        emit shutdownDue(t);
    }
}

QDateTime Scheduler::triggerDateTime(const models::ShutdownTask& t, const QDateTime& now) const {
    QTime time = QTime::fromString(t.time, "HH:mm");
    if (!time.isValid()) return QDateTime();
    // If today's time already passed, the task is not imminent
    QDateTime todayTrigger(now.date(), time);
    return todayTrigger;
}

void Scheduler::evaluateShutdownWarnings(const QDateTime& now) {
    const auto tasks = dal::ShutdownTaskDao().findAll();
    for (const auto& t : tasks) {
        if (!t.enabled) continue;
        if (!CycleUtils::occursOnDate(now.date(), t.cycleMode, t.cycleData)) continue;
        QDateTime trigger = triggerDateTime(t, now);
        if (!trigger.isValid()) continue;
        qint64 secsLeft = now.secsTo(trigger);
        if (secsLeft < 0 || secsLeft > t.advanceSeconds) continue;
        emit shutdownWarning(t, static_cast<int>(secsLeft));
    }
}

void Scheduler::evaluateIntervalTasks(const QDateTime& now) {
    const auto progs = dal::RunProgramTaskDao().findAll();
    for (const auto& t : progs) {
        if (!t.enabled) continue;
        if (t.cycleMode != 5) continue; // Only interval mode

        // Check if we have a scheduled next fire time
        QDateTime nextFire = intervalNextFire_.value(t.uuid);

        // Calculate next fire time if not set
        if (!nextFire.isValid()) {
            QDateTime next = CycleUtils::nextOccurrence(now, t.cycleMode, t.cycleData,
                                                        t.time, t.rangeStart, t.rangeEnd);
            if (!next.isValid()) continue;
            intervalNextFire_[t.uuid] = next;
            nextFire = next;
        }

        // Trigger if current time has reached the scheduled fire time
        if (now >= nextFire) {
            // Use second-level dedup to prevent double-firing
            QString key = t.uuid + "@" + nextFire.toString("yyyyMMddHHmmss");
            if (!firedKeys_.contains(key)) {
                firedKeys_.insert(key);
                emit runProgramTriggered(t);
            }

            // Calculate the next fire time from the current fire time + 1 second
            // This ensures consistent interval spacing from the anchor
            QDateTime subsequent = CycleUtils::nextOccurrence(nextFire.addSecs(1), t.cycleMode,
                                                              t.cycleData, t.time,
                                                              t.rangeStart, t.rangeEnd);
            if (subsequent.isValid()) {
                intervalNextFire_[t.uuid] = subsequent;
            }
        }
    }
}

// ── StopwatchService ──

StopwatchService::StopwatchService(QObject* parent)
    : QObject(parent)
{
}

StopwatchService& StopwatchService::instance() {
    static StopwatchService shared;
    return shared;
}

qint64 StopwatchService::elapsedMs() const {
    if (state_ == State::Running) {
        return accumulatedMs_ + segmentStart_.msecsTo(QDateTime::currentDateTime());
    }
    return accumulatedMs_;
}

void StopwatchService::start() {
    if (state_ == State::Running) return;
    if (state_ == State::Stopped) {
        accumulatedMs_ = 0;
        laps_.clear();
    }
    segmentStart_ = QDateTime::currentDateTime();
    state_ = State::Running;
    emit started();
}

void StopwatchService::pause() {
    if (state_ != State::Running) return;
    accumulatedMs_ += segmentStart_.msecsTo(QDateTime::currentDateTime());
    state_ = State::Paused;
    emit paused();
}

void StopwatchService::reset() {
    state_ = State::Stopped;
    accumulatedMs_ = 0;
    laps_.clear();
    emit resetDone();
}

qint64 StopwatchService::lap() {
    qint64 total = elapsedMs();
    laps_.append(total);
    emit lapped(total);
    return total;
}

} // namespace mcclock::services
