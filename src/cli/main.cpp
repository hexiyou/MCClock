// MCClock-CLI: command line interface sharing the same SQLite database
// as the GUI application (WAL mode + busy timeout for concurrency).
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDateTime>
#include <QDir>

#include "core/dal/database.h"
#include "core/dal/settings_manager.h"
#include "core/utils/platform_utils.h"
#include "core/services/business_services.h"

using namespace mcclock::models;
using namespace mcclock::services;
using mcclock::dal::SettingsManager;
using mcclock::utils::PlatformUtils;

namespace {

QTextStream out(stdout);
QTextStream errStream(stderr);

// ── Cycle helpers ──
int cycleNameToMode(const QString& name) {
    QString n = name.toLower();
    if (n == "once") return static_cast<int>(CycleMode::Once);
    if (n == "daily") return static_cast<int>(CycleMode::Daily);
    if (n == "weekly") return static_cast<int>(CycleMode::Weekly);
    if (n == "monthly") return static_cast<int>(CycleMode::Monthly);
    if (n == "yearly") return static_cast<int>(CycleMode::Yearly);
    if (n == "interval") return static_cast<int>(CycleMode::Interval);
    return -1;
}

QString cycleModeToName(int mode) {
    switch (static_cast<CycleMode>(mode)) {
    case CycleMode::Once: return "once";
    case CycleMode::Daily: return "daily";
    case CycleMode::Weekly: return "weekly";
    case CycleMode::Monthly: return "monthly";
    case CycleMode::Yearly: return "yearly";
    case CycleMode::Interval: return "interval";
    }
    return "?";
}

QString shutdownOptionToName(int opt) {
    switch (static_cast<ShutdownOption>(opt)) {
    case ShutdownOption::ForceShutdown: return "force-shutdown";
    case ShutdownOption::NormalShutdown: return "shutdown";
    case ShutdownOption::Restart: return "restart";
    case ShutdownOption::Logoff: return "logoff";
    }
    return "?";
}

int shutdownNameToOption(const QString& name) {
    QString n = name.toLower();
    if (n == "force-shutdown" || n == "force") return static_cast<int>(ShutdownOption::ForceShutdown);
    if (n == "shutdown" || n == "normal") return static_cast<int>(ShutdownOption::NormalShutdown);
    if (n == "restart") return static_cast<int>(ShutdownOption::Restart);
    if (n == "logoff") return static_cast<int>(ShutdownOption::Logoff);
    return -1;
}

QString shortUuid(const QString& uuid) { return uuid.left(8); }

QString boolMark(bool v) { return v ? "Y" : "N"; }

void printJson(const QJsonValue& v) {
    QJsonDocument doc(v.isObject() ? QJsonDocument(v.toObject()) : QJsonDocument(v.toArray()));
    out << doc.toJson(QJsonDocument::Indented) << "\n";
}

// ── Export / import helpers ──
template <typename List>
bool exportToFile(const List& items, const QString& path) {
    QJsonArray arr;
    for (const auto& m : items) arr.push_back(m.toJson());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        errStream << "Error: cannot write file: " << path << "\n";
        return false;
    }
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    out << "Exported " << arr.size() << " item(s) to " << path << "\n";
    return true;
}

QJsonArray readJsonArrayFile(const QString& path, bool& ok) {
    ok = false;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        errStream << "Error: cannot read file: " << path << "\n";
        return {};
    }
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isArray()) {
        errStream << "Error: invalid JSON array file: " << pe.errorString() << "\n";
        return {};
    }
    ok = true;
    return doc.array();
}

// ════════════════════════ alarm ════════════════════════

void alarmListTable(const QList<Alarm>& alarms) {
    out << "UUID      EN TIME  CYCLE    LABEL\n";
    for (const auto& a : alarms) {
        out << shortUuid(a.uuid).leftJustified(10)
            << boolMark(a.enabled).leftJustified(3)
            << a.time.leftJustified(8)
            << cycleModeToName(a.cycleMode).leftJustified(9)
            << a.label << "\n";
    }
    out << "(" << alarms.size() << " item(s))\n";
}

int handleAlarm(const QString& action, QCommandLineParser& p,
                const QCommandLineOption& uuidOpt, const QCommandLineOption& labelOpt,
                const QCommandLineOption& timeOpt, const QCommandLineOption& cycleOpt,
                const QCommandLineOption& cycleDataOpt, const QCommandLineOption& fileOpt,
                bool jsonOutput) {
    AlarmService svc;

    if (action.isEmpty() || action == "list") {
        auto items = svc.findAll();
        if (jsonOutput) {
            QJsonArray arr;
            for (const auto& a : items) arr.push_back(a.toJson());
            printJson(arr);
        } else {
            alarmListTable(items);
        }
        return 0;
    }
    if (action == "deleted") {
        auto items = svc.findDeleted();
        if (jsonOutput) {
            QJsonArray arr;
            for (const auto& a : items) arr.push_back(a.toJson());
            printJson(arr);
        } else {
            alarmListTable(items);
        }
        return 0;
    }
    if (action == "add") {
        Alarm a;
        if (!p.isSet(timeOpt)) { errStream << "Error: --time is required\n"; return 1; }
        a.time = p.value(timeOpt);
        a.label = p.value(labelOpt);
        if (p.isSet(cycleOpt)) {
            int m = cycleNameToMode(p.value(cycleOpt));
            if (m < 0) { errStream << "Error: invalid --cycle\n"; return 1; }
            a.cycleMode = m;
        }
        a.cycleData = p.value(cycleDataOpt);
        auto saved = svc.add(a);
        out << "Added alarm " << saved.uuid << "\n";
        return 0;
    }
    if (action == "edit") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        Alarm a = svc.findByUuid(p.value(uuidOpt));
        if (a.uuid.isEmpty()) { errStream << "Error: alarm not found\n"; return 1; }
        if (p.isSet(timeOpt)) a.time = p.value(timeOpt);
        if (p.isSet(labelOpt)) a.label = p.value(labelOpt);
        if (p.isSet(cycleOpt)) {
            int m = cycleNameToMode(p.value(cycleOpt));
            if (m < 0) { errStream << "Error: invalid --cycle\n"; return 1; }
            a.cycleMode = m;
        }
        if (p.isSet(cycleDataOpt)) a.cycleData = p.value(cycleDataOpt);
        if (svc.update(a)) { out << "Updated alarm " << a.uuid << "\n"; return 0; }
        errStream << "Error: update failed\n";
        return 1;
    }
    if (action == "delete") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.moveToRecycleBin(p.value(uuidOpt))) { out << "Moved to recycle bin\n"; return 0; }
        errStream << "Error: delete failed\n";
        return 1;
    }
    if (action == "restore") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.restore(p.value(uuidOpt))) { out << "Restored\n"; return 0; }
        errStream << "Error: restore failed\n";
        return 1;
    }
    if (action == "purge") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.hardDelete(p.value(uuidOpt))) { out << "Deleted permanently\n"; return 0; }
        errStream << "Error: purge failed\n";
        return 1;
    }
    if (action == "clear-recycle") {
        if (svc.clearRecycleBin()) { out << "Recycle bin cleared\n"; return 0; }
        errStream << "Error: clear failed\n";
        return 1;
    }
    if (action == "enable" || action == "disable") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.setEnabled(p.value(uuidOpt), action == "enable")) {
            out << (action == "enable" ? "Enabled\n" : "Disabled\n");
            return 0;
        }
        errStream << "Error: failed\n";
        return 1;
    }
    if (action == "export") {
        QString file = p.isSet(fileOpt) ? p.value(fileOpt) : QStringLiteral("alarms.json");
        return exportToFile(svc.findAll(), file) ? 0 : 1;
    }
    if (action == "import") {
        if (!p.isSet(fileOpt)) { errStream << "Error: --file is required\n"; return 1; }
        bool ok;
        QJsonArray arr = readJsonArrayFile(p.value(fileOpt), ok);
        if (!ok) return 1;
        int count = 0;
        for (const auto& v : arr) {
            Alarm a = Alarm::fromJson(v.toObject());
            a.uuid.clear();  // assign new uuid
            svc.add(a);
            ++count;
        }
        out << "Imported " << count << " alarm(s)\n";
        return 0;
    }
    errStream << "Error: unknown alarm action: " << action << "\n";
    return 1;
}

// ════════════════════════ birthday ════════════════════════

int handleBirthday(const QString& action, QCommandLineParser& p,
                   const QCommandLineOption& uuidOpt, const QCommandLineOption& nameOpt,
                   const QCommandLineOption& dateOpt, const QCommandLineOption& lunarOpt,
                   const QCommandLineOption& remindTimeOpt, const QCommandLineOption& advanceOpt,
                   const QCommandLineOption& labelOpt, const QCommandLineOption& fileOpt,
                   bool jsonOutput) {
    BirthdayService svc;

    if (action.isEmpty() || action == "list") {
        auto items = svc.findAll();
        if (jsonOutput) {
            QJsonArray arr;
            for (const auto& b : items) arr.push_back(b.toJson());
            printJson(arr);
        } else {
            out << "UUID      NAME             DATE       TYPE    DAYS\n";
            for (const auto& b : items) {
                QString date = b.isLunar
                    ? QStringLiteral("L%1-%2").arg(b.lunarMonth, 2, 10, QLatin1Char('0')).arg(b.lunarDay, 2, 10, QLatin1Char('0'))
                    : QStringLiteral("%1-%2").arg(b.solarMonth, 2, 10, QLatin1Char('0')).arg(b.solarDay, 2, 10, QLatin1Char('0'));
                out << shortUuid(b.uuid).leftJustified(10)
                    << b.name.leftJustified(17)
                    << date.leftJustified(11)
                    << (b.isLunar ? QString("lunar") : QString("solar")).leftJustified(8)
                    << svc.daysUntilBirthday(b) << "\n";
            }
            out << "(" << items.size() << " item(s))\n";
        }
        return 0;
    }

    auto applyDate = [&](Birthday& b) -> bool {
        if (!p.isSet(dateOpt)) return true;
        QDate d = QDate::fromString(p.value(dateOpt), "yyyy-MM-dd");
        if (!d.isValid()) {
            d = QDate::fromString(p.value(dateOpt), "MM-dd");
            if (!d.isValid()) { errStream << "Error: invalid --date (yyyy-MM-dd or MM-dd)\n"; return false; }
        }
        if (p.isSet(lunarOpt)) {
            b.isLunar = true;
            b.lunarMonth = d.month();
            b.lunarDay = d.day();
        } else {
            b.isLunar = false;
            b.solarYear = d.year();
            b.solarMonth = d.month();
            b.solarDay = d.day();
        }
        return true;
    };

    if (action == "add") {
        Birthday b;
        b.name = p.value(nameOpt);
        if (b.name.isEmpty()) { errStream << "Error: --name is required\n"; return 1; }
        if (!applyDate(b)) return 1;
        if (p.isSet(remindTimeOpt)) b.remindTime = p.value(remindTimeOpt);
        if (p.isSet(advanceOpt)) b.advanceDays = p.value(advanceOpt).toInt();
        if (p.isSet(labelOpt)) b.label = p.value(labelOpt);
        auto saved = svc.add(b);
        out << "Added birthday " << saved.uuid << "\n";
        return 0;
    }
    if (action == "edit") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        Birthday b = svc.findByUuid(p.value(uuidOpt));
        if (b.uuid.isEmpty()) { errStream << "Error: birthday not found\n"; return 1; }
        if (p.isSet(nameOpt)) b.name = p.value(nameOpt);
        if (!applyDate(b)) return 1;
        if (p.isSet(remindTimeOpt)) b.remindTime = p.value(remindTimeOpt);
        if (p.isSet(advanceOpt)) b.advanceDays = p.value(advanceOpt).toInt();
        if (p.isSet(labelOpt)) b.label = p.value(labelOpt);
        if (svc.update(b)) { out << "Updated birthday " << b.uuid << "\n"; return 0; }
        errStream << "Error: update failed\n";
        return 1;
    }
    if (action == "delete") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.remove(p.value(uuidOpt))) { out << "Deleted\n"; return 0; }
        errStream << "Error: delete failed\n";
        return 1;
    }
    if (action == "export") {
        QString file = p.isSet(fileOpt) ? p.value(fileOpt) : QStringLiteral("birthdays.json");
        return exportToFile(svc.findAll(), file) ? 0 : 1;
    }
    if (action == "import") {
        if (!p.isSet(fileOpt)) { errStream << "Error: --file is required\n"; return 1; }
        bool ok;
        QJsonArray arr = readJsonArrayFile(p.value(fileOpt), ok);
        if (!ok) return 1;
        int count = 0;
        for (const auto& v : arr) {
            Birthday b = Birthday::fromJson(v.toObject());
            b.uuid.clear();
            svc.add(b);
            ++count;
        }
        out << "Imported " << count << " birthday(s)\n";
        return 0;
    }
    errStream << "Error: unknown birthday action: " << action << "\n";
    return 1;
}

// ════════════════════════ shutdown ════════════════════════

int handleShutdown(const QString& action, QCommandLineParser& p,
                   const QCommandLineOption& uuidOpt, const QCommandLineOption& labelOpt,
                   const QCommandLineOption& timeOpt, const QCommandLineOption& cycleOpt,
                   const QCommandLineOption& optionOpt, const QCommandLineOption& advanceOpt,
                   const QCommandLineOption& yesOpt, const QCommandLineOption& fileOpt,
                   bool jsonOutput) {
    ShutdownService svc;

    if (action.isEmpty() || action == "list") {
        auto items = svc.findAll();
        if (jsonOutput) {
            QJsonArray arr;
            for (const auto& t : items) arr.push_back(t.toJson());
            printJson(arr);
        } else {
            out << "UUID      EN TIME  CYCLE    OPTION           LABEL\n";
            for (const auto& t : items) {
                out << shortUuid(t.uuid).leftJustified(10)
                    << boolMark(t.enabled).leftJustified(3)
                    << t.time.leftJustified(8)
                    << cycleModeToName(t.cycleMode).leftJustified(9)
                    << shutdownOptionToName(t.shutdownOption).leftJustified(17)
                    << t.label << "\n";
            }
            out << "(" << items.size() << " item(s))\n";
        }
        return 0;
    }
    if (action == "add") {
        ShutdownTask t;
        if (!p.isSet(timeOpt)) { errStream << "Error: --time is required\n"; return 1; }
        t.time = p.value(timeOpt);
        t.label = p.value(labelOpt);
        if (p.isSet(cycleOpt)) {
            int m = cycleNameToMode(p.value(cycleOpt));
            if (m < 0) { errStream << "Error: invalid --cycle\n"; return 1; }
            t.cycleMode = m;
        }
        if (p.isSet(optionOpt)) {
            int o = shutdownNameToOption(p.value(optionOpt));
            if (o < 0) { errStream << "Error: invalid --option\n"; return 1; }
            t.shutdownOption = o;
        }
        if (p.isSet(advanceOpt)) t.advanceSeconds = p.value(advanceOpt).toInt();
        auto saved = svc.add(t);
        out << "Added shutdown task " << saved.uuid << "\n";
        return 0;
    }
    if (action == "edit") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        ShutdownTask t = svc.findByUuid(p.value(uuidOpt));
        if (t.uuid.isEmpty()) { errStream << "Error: task not found\n"; return 1; }
        if (p.isSet(timeOpt)) t.time = p.value(timeOpt);
        if (p.isSet(labelOpt)) t.label = p.value(labelOpt);
        if (p.isSet(cycleOpt)) {
            int m = cycleNameToMode(p.value(cycleOpt));
            if (m < 0) { errStream << "Error: invalid --cycle\n"; return 1; }
            t.cycleMode = m;
        }
        if (p.isSet(optionOpt)) {
            int o = shutdownNameToOption(p.value(optionOpt));
            if (o < 0) { errStream << "Error: invalid --option\n"; return 1; }
            t.shutdownOption = o;
        }
        if (p.isSet(advanceOpt)) t.advanceSeconds = p.value(advanceOpt).toInt();
        if (svc.update(t)) { out << "Updated shutdown task " << t.uuid << "\n"; return 0; }
        errStream << "Error: update failed\n";
        return 1;
    }
    if (action == "delete") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.remove(p.value(uuidOpt))) { out << "Deleted\n"; return 0; }
        errStream << "Error: delete failed\n";
        return 1;
    }
    if (action == "enable" || action == "disable") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.setEnabled(p.value(uuidOpt), action == "enable")) {
            out << (action == "enable" ? "Enabled\n" : "Disabled\n");
            return 0;
        }
        errStream << "Error: failed\n";
        return 1;
    }
    if (action == "run") {
        if (!p.isSet(yesOpt)) {
            errStream << "Error: executing a shutdown task requires --yes confirmation\n";
            return 1;
        }
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        ShutdownTask t = svc.findByUuid(p.value(uuidOpt));
        if (t.uuid.isEmpty()) { errStream << "Error: task not found\n"; return 1; }
        if (svc.executeNow(t)) { out << "Executed\n"; return 0; }
        errStream << "Error: execute failed\n";
        return 1;
    }
    if (action == "export") {
        QString file = p.isSet(fileOpt) ? p.value(fileOpt) : QStringLiteral("shutdown-tasks.json");
        return exportToFile(svc.findAll(), file) ? 0 : 1;
    }
    if (action == "import") {
        if (!p.isSet(fileOpt)) { errStream << "Error: --file is required\n"; return 1; }
        bool ok;
        QJsonArray arr = readJsonArrayFile(p.value(fileOpt), ok);
        if (!ok) return 1;
        int count = 0;
        for (const auto& v : arr) {
            ShutdownTask t = ShutdownTask::fromJson(v.toObject());
            t.uuid.clear();
            svc.add(t);
            ++count;
        }
        out << "Imported " << count << " task(s)\n";
        return 0;
    }
    errStream << "Error: unknown shutdown action: " << action << "\n";
    return 1;
}

// ════════════════════════ run (program tasks) ════════════════════════

int handleRun(const QString& action, QCommandLineParser& p,
              const QCommandLineOption& uuidOpt, const QCommandLineOption& labelOpt,
              const QCommandLineOption& timeOpt, const QCommandLineOption& cycleOpt,
              const QCommandLineOption& pathOpt, const QCommandLineOption& argsOpt,
              const QCommandLineOption& yesOpt, const QCommandLineOption& fileOpt,
              bool jsonOutput) {
    RunProgramService svc;

    if (action.isEmpty() || action == "list") {
        auto items = svc.findAll();
        if (jsonOutput) {
            QJsonArray arr;
            for (const auto& t : items) arr.push_back(t.toJson());
            printJson(arr);
        } else {
            out << "UUID      EN TIME  CYCLE    PROGRAM\n";
            for (const auto& t : items) {
                out << shortUuid(t.uuid).leftJustified(10)
                    << boolMark(t.enabled).leftJustified(3)
                    << t.time.leftJustified(8)
                    << cycleModeToName(t.cycleMode).leftJustified(9)
                    << t.programPath << "\n";
            }
            out << "(" << items.size() << " item(s))\n";
        }
        return 0;
    }
    if (action == "add") {
        RunProgramTask t;
        if (!p.isSet(pathOpt)) { errStream << "Error: --path is required\n"; return 1; }
        t.programPath = p.value(pathOpt);
        t.arguments = p.value(argsOpt);
        t.time = p.isSet(timeOpt) ? p.value(timeOpt) : QStringLiteral("00:00");
        t.label = p.value(labelOpt);
        if (p.isSet(cycleOpt)) {
            int m = cycleNameToMode(p.value(cycleOpt));
            if (m < 0) { errStream << "Error: invalid --cycle\n"; return 1; }
            t.cycleMode = m;
        }
        auto saved = svc.add(t);
        out << "Added run-program task " << saved.uuid << "\n";
        return 0;
    }
    if (action == "edit") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        RunProgramTask t = svc.findByUuid(p.value(uuidOpt));
        if (t.uuid.isEmpty()) { errStream << "Error: task not found\n"; return 1; }
        if (p.isSet(pathOpt)) t.programPath = p.value(pathOpt);
        if (p.isSet(argsOpt)) t.arguments = p.value(argsOpt);
        if (p.isSet(timeOpt)) t.time = p.value(timeOpt);
        if (p.isSet(labelOpt)) t.label = p.value(labelOpt);
        if (p.isSet(cycleOpt)) {
            int m = cycleNameToMode(p.value(cycleOpt));
            if (m < 0) { errStream << "Error: invalid --cycle\n"; return 1; }
            t.cycleMode = m;
        }
        if (svc.update(t)) { out << "Updated run-program task " << t.uuid << "\n"; return 0; }
        errStream << "Error: update failed\n";
        return 1;
    }
    if (action == "delete") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.remove(p.value(uuidOpt))) { out << "Deleted\n"; return 0; }
        errStream << "Error: delete failed\n";
        return 1;
    }
    if (action == "enable" || action == "disable") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.setEnabled(p.value(uuidOpt), action == "enable")) {
            out << (action == "enable" ? "Enabled\n" : "Disabled\n");
            return 0;
        }
        errStream << "Error: failed\n";
        return 1;
    }
    if (action == "run") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        RunProgramTask t = svc.findByUuid(p.value(uuidOpt));
        if (t.uuid.isEmpty()) { errStream << "Error: task not found\n"; return 1; }
        Q_UNUSED(yesOpt);
        if (svc.executeNow(t)) { out << "Launched\n"; return 0; }
        errStream << "Error: launch failed\n";
        return 1;
    }
    if (action == "export") {
        QString file = p.isSet(fileOpt) ? p.value(fileOpt) : QStringLiteral("run-programs.json");
        return exportToFile(svc.findAll(), file) ? 0 : 1;
    }
    if (action == "import") {
        if (!p.isSet(fileOpt)) { errStream << "Error: --file is required\n"; return 1; }
        bool ok;
        QJsonArray arr = readJsonArrayFile(p.value(fileOpt), ok);
        if (!ok) return 1;
        int count = 0;
        for (const auto& v : arr) {
            RunProgramTask t = RunProgramTask::fromJson(v.toObject());
            t.uuid.clear();
            svc.add(t);
            ++count;
        }
        out << "Imported " << count << " task(s)\n";
        return 0;
    }
    errStream << "Error: unknown run action: " << action << "\n";
    return 1;
}

// ════════════════════════ countdown ════════════════════════

int handleCountdown(const QString& action, QCommandLineParser& p,
                    const QCommandLineOption& uuidOpt, const QCommandLineOption& labelOpt,
                    const QCommandLineOption& minutesOpt, const QCommandLineOption& targetOpt,
                    const QCommandLineOption& fileOpt, bool jsonOutput) {
    CountdownService svc;

    if (action.isEmpty() || action == "list") {
        auto items = svc.findAll();
        if (jsonOutput) {
            QJsonArray arr;
            for (const auto& c : items) arr.push_back(c.toJson());
            printJson(arr);
        } else {
            out << "UUID      EN MODE     SPEC                LABEL\n";
            for (const auto& c : items) {
                QString spec = c.mode == static_cast<int>(CountdownMode::Relative)
                    ? QStringLiteral("%1 min").arg(c.totalSeconds / 60)
                    : c.targetDatetime.left(16);
                out << shortUuid(c.uuid).leftJustified(10)
                    << boolMark(c.enabled).leftJustified(3)
                                        << (c.mode == static_cast<int>(CountdownMode::Relative) ? QString("relative") : QString("absolute")).leftJustified(9)
                    << spec.leftJustified(20)
                    << c.label << "\n";
            }
            out << "(" << items.size() << " item(s))\n";
        }
        return 0;
    }
    if (action == "add") {
        Countdown c;
        if (p.isSet(targetOpt)) {
            c.mode = static_cast<int>(CountdownMode::Absolute);
            c.targetDatetime = p.value(targetOpt);
        } else if (p.isSet(minutesOpt)) {
            c.mode = static_cast<int>(CountdownMode::Relative);
            c.totalSeconds = p.value(minutesOpt).toInt() * 60;
            c.remainingSeconds = c.totalSeconds;
        } else {
            errStream << "Error: --minutes or --target is required\n";
            return 1;
        }
        c.label = p.value(labelOpt);
        auto saved = svc.add(c);
        out << "Added countdown " << saved.uuid << "\n";
        return 0;
    }
    if (action == "edit") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        Countdown c = svc.findByUuid(p.value(uuidOpt));
        if (c.uuid.isEmpty()) { errStream << "Error: countdown not found\n"; return 1; }
        if (p.isSet(targetOpt)) {
            c.mode = static_cast<int>(CountdownMode::Absolute);
            c.targetDatetime = p.value(targetOpt);
        }
        if (p.isSet(minutesOpt)) {
            c.mode = static_cast<int>(CountdownMode::Relative);
            c.totalSeconds = p.value(minutesOpt).toInt() * 60;
            c.remainingSeconds = c.totalSeconds;
        }
        if (p.isSet(labelOpt)) c.label = p.value(labelOpt);
        if (svc.update(c)) { out << "Updated countdown " << c.uuid << "\n"; return 0; }
        errStream << "Error: update failed\n";
        return 1;
    }
    if (action == "delete") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.remove(p.value(uuidOpt))) { out << "Deleted\n"; return 0; }
        errStream << "Error: delete failed\n";
        return 1;
    }
    if (action == "enable" || action == "disable") {
        if (!p.isSet(uuidOpt)) { errStream << "Error: --uuid is required\n"; return 1; }
        if (svc.setEnabled(p.value(uuidOpt), action == "enable")) {
            out << (action == "enable" ? "Enabled\n" : "Disabled\n");
            return 0;
        }
        errStream << "Error: failed\n";
        return 1;
    }
    if (action == "export") {
        QString file = p.isSet(fileOpt) ? p.value(fileOpt) : QStringLiteral("countdowns.json");
        return exportToFile(svc.findAll(), file) ? 0 : 1;
    }
    if (action == "import") {
        if (!p.isSet(fileOpt)) { errStream << "Error: --file is required\n"; return 1; }
        bool ok;
        QJsonArray arr = readJsonArrayFile(p.value(fileOpt), ok);
        if (!ok) return 1;
        int count = 0;
        for (const auto& v : arr) {
            Countdown c = Countdown::fromJson(v.toObject());
            c.uuid.clear();
            svc.add(c);
            ++count;
        }
        out << "Imported " << count << " countdown(s)\n";
        return 0;
    }
    errStream << "Error: unknown countdown action: " << action << "\n";
    return 1;
}

// ════════════════════════ settings ════════════════════════

int handleSettings(const QString& action, const QStringList& args, bool jsonOutput) {
    auto& s = SettingsManager::instance();

    if (action.isEmpty() || action == "list") {
        if (jsonOutput) printJson(s.rootObject());
        else out << QJsonDocument(s.rootObject()).toJson(QJsonDocument::Indented) << "\n";
        return 0;
    }
    if (action == "get") {
        if (args.size() < 3) { errStream << "Error: usage: settings get <path> (dot-separated)\n"; return 1; }
        QStringList path = args.value(2).split('.', Qt::SkipEmptyParts);
        QJsonValue v = s.get(path);
        if (v.isUndefined()) { errStream << "Error: key not found: " << args.value(2) << "\n"; return 1; }
        if (jsonOutput) {
            QJsonObject obj;
            obj["key"] = args.value(2);
            obj["value"] = v;
            printJson(obj);
        } else {
            if (v.isString()) out << v.toString() << "\n";
            else if (v.isDouble() || v.isBool()) out << v.toVariant().toString() << "\n";
            else out << QJsonDocument(v.isArray() ? QJsonDocument(v.toArray()) : QJsonDocument(v.toObject())).toJson(QJsonDocument::Compact) << "\n";
        }
        return 0;
    }
    if (action == "set") {
        if (args.size() < 4) { errStream << "Error: usage: settings set <path> <value>\n"; return 1; }
        QStringList path = args.value(2).split('.', Qt::SkipEmptyParts);
        QString raw = args.value(3);
        // Interpret value: bool / number / JSON object/array, fallback to string
        QJsonValue value = QJsonValue(raw);
        if (raw == "true") value = QJsonValue(true);
        else if (raw == "false") value = QJsonValue(false);
        else {
            bool numOk = false;
            double d = raw.toDouble(&numOk);
            if (numOk) {
                value = QJsonValue(d);
            } else {
                QJsonParseError pe;
                QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8(), &pe);
                if (pe.error == QJsonParseError::NoError) {
                    if (doc.isObject()) value = QJsonValue(doc.object());
                    else if (doc.isArray()) value = QJsonValue(doc.array());
                }
            }
        }
        s.set(path, value);
        if (!s.save()) { errStream << "Error: failed to save settings\n"; return 1; }
        out << "OK\n";
        return 0;
    }
    errStream << "Error: unknown settings action: " << action << "\n";
    return 1;
}

// ════════════════════════ system ════════════════════════

int handleSystem(const QString& action, QCommandLineParser& p,
                 const QCommandLineOption& yesOpt, const QCommandLineOption& optionOpt,
                 const QCommandLineOption& fileOpt, bool jsonOutput) {
    if (action.isEmpty() || action == "info") {
        QJsonObject info;
        info["app"] = "MCClock";
        info["version"] = QCoreApplication::applicationVersion();
        info["datetime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        info["database"] = PlatformUtils::databasePath();
        info["settings"] = PlatformUtils::settingsPath();
        info["autoStart"] = PlatformUtils::isAutoStartEnabled();
        if (jsonOutput) printJson(info);
        else {
            out << "App:        MCClock " << info["version"].toString() << "\n";
            out << "Time:       " << info["datetime"].toString() << "\n";
            out << "Database:   " << info["database"].toString() << "\n";
            out << "Settings:   " << info["settings"].toString() << "\n";
            out << "Auto-start: " << (info["autoStart"].toBool() ? "enabled" : "disabled") << "\n";
        }
        return 0;
    }
    if (action == "autostart") {
        // Usage: system autostart on|off
        QStringList pos = p.positionalArguments();
        QString val = pos.value(2).toLower();
        if (val != "on" && val != "off") {
            errStream << "Error: usage: system autostart on|off\n";
            return 1;
        }
        bool enable = (val == "on");
        if (!PlatformUtils::setAutoStart(enable)) { errStream << "Error: failed\n"; return 1; }
        SettingsManager::instance().setAutoStart(enable);
        SettingsManager::instance().save();
        out << (enable ? "Auto-start enabled\n" : "Auto-start disabled\n");
        return 0;
    }
    if (action == "shutdown" || action == "restart" || action == "logoff") {
        if (!p.isSet(yesOpt)) {
            errStream << "Error: dangerous operation requires --yes confirmation\n";
            return 1;
        }
        int option;
        if (action == "restart") option = static_cast<int>(ShutdownOption::Restart);
        else if (action == "logoff") option = static_cast<int>(ShutdownOption::Logoff);
        else if (p.isSet(optionOpt) && shutdownNameToOption(p.value(optionOpt)) == static_cast<int>(ShutdownOption::ForceShutdown))
            option = static_cast<int>(ShutdownOption::ForceShutdown);
        else option = static_cast<int>(ShutdownOption::NormalShutdown);
        out << "Executing " << shutdownOptionToName(option) << "...\n";
        return PlatformUtils::executeShutdown(option) ? 0 : 1;
    }
    if (action == "backup") {
        QString file = p.isSet(fileOpt)
            ? p.value(fileOpt)
            : QStringLiteral("mcclock-backup-%1.zip")
                  .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
        QString dataDir = PlatformUtils::appDataPath();

        // Flush the WAL so the db file is self-contained
        mcclock::dal::Database::instance().execute("PRAGMA wal_checkpoint(TRUNCATE);");

        QStringList files;
        for (const QString& name : {QStringLiteral("mcclock.db"), QStringLiteral("settings.json")}) {
            if (QFile::exists(dataDir + "/" + name)) files << name;
        }
        if (files.isEmpty()) { errStream << "Error: no data to back up\n"; return 1; }

        QProcess tar;
        tar.start("tar.exe", QStringList{"-a", "-c", "-f", QDir::toNativeSeparators(file),
                                         "-C", QDir::toNativeSeparators(dataDir)} + files);
        tar.waitForFinished(30000);
        if (tar.exitCode() != 0) {
            errStream << "Error: backup failed: " << tar.readAllStandardError() << "\n";
            return 1;
        }
        out << "Backup created: " << file << "\n";
        return 0;
    }
    if (action == "restore") {
        if (!p.isSet(fileOpt)) { errStream << "Error: --file is required\n"; return 1; }
        if (!p.isSet(yesOpt)) {
            errStream << "Error: restore overwrites current data, requires --yes confirmation\n";
            return 1;
        }
        QString file = p.value(fileOpt);
        if (!QFile::exists(file)) { errStream << "Error: file not found: " << file << "\n"; return 1; }
        QString dataDir = PlatformUtils::appDataPath();

        // Close the database before overwriting it
        mcclock::dal::Database::instance().close();

        QProcess tar;
        tar.start("tar.exe", QStringList{"-x", "-f", QDir::toNativeSeparators(file),
                                         "-C", QDir::toNativeSeparators(dataDir)});
        tar.waitForFinished(30000);
        if (tar.exitCode() != 0) {
            errStream << "Error: restore failed: " << tar.readAllStandardError() << "\n";
            return 1;
        }

        // Reopen the restored database
        if (!mcclock::dal::Database::instance().initialize(PlatformUtils::databasePath())) {
            errStream << "Error: restored database cannot be opened\n";
            return 1;
        }
        SettingsManager::instance().load(PlatformUtils::settingsPath());
        out << "Restore completed from: " << file << "\n";
        return 0;
    }
    errStream << "Error: unknown system action: " << action << "\n";
    return 1;
}

void printUsage() {
    out << "MCClock CLI v1.0.0\n\n";
    out << "Usage: MCClock-CLI.exe <module> <action> [options]\n\n";
    out << "Modules:\n";
    out << "  alarm       Alarm management\n";
    out << "  birthday    Birthday management\n";
    out << "  shutdown    Scheduled shutdown tasks\n";
    out << "  run         Run program tasks\n";
    out << "  countdown   Countdown timers\n";
    out << "  settings    Application settings\n";
    out << "  system      System operations\n\n";
    out << "Common actions:\n";
    out << "  list        List items (default action)\n";
    out << "  add         Add item\n";
    out << "  edit        Edit item (--uuid required)\n";
    out << "  delete      Delete item (--uuid required)\n";
    out << "  enable/disable  Toggle item (--uuid required)\n";
    out << "  export      Export to JSON file (--file)\n";
    out << "  import      Import from JSON file (--file)\n\n";
    out << "Module-specific actions:\n";
    out << "  alarm:      deleted, restore, purge, clear-recycle\n";
    out << "  birthday:   (list/add/edit/delete/export/import)\n";
    out << "  shutdown:   run (--yes --uuid)\n";
    out << "  run:        run (--uuid)\n";
    out << "  settings:   get <path>, set <path> <value>, list\n";
    out << "  system:     info, autostart on|off, shutdown/restart/logoff (--yes),\n";
    out << "              backup [--file x.zip], restore --file x.zip --yes\n\n";
    out << "Options:\n";
    out << "  --json          JSON output format\n";
    out << "  --uuid <uuid>   Item UUID\n";
    out << "  --label <text>  Item label\n";
    out << "  --time <HH:mm>  Trigger time\n";
    out << "  --cycle <mode>  once|daily|weekly|monthly|yearly|interval\n";
    out << "  --cycle-data <json>  Cycle parameters (JSON)\n";
    out << "  --name <text>   Birthday name\n";
    out << "  --date <d>      Date yyyy-MM-dd or MM-dd\n";
    out << "  --lunar         Treat --date as lunar month-day\n";
    out << "  --option <o>    force-shutdown|shutdown|restart|logoff\n";
    out << "  --advance <n>   Advance seconds/days\n";
    out << "  --minutes <n>   Countdown duration (minutes)\n";
    out << "  --target <dt>   Countdown target datetime (ISO8601)\n";
    out << "  --path <p>      Program path or URL\n";
    out << "  --args <a>      Program arguments\n";
    out << "  --file <f>      Export/import file path\n";
    out << "  --yes           Confirm dangerous operations\n\n";
    out << "Examples:\n";
    out << "  MCClock-CLI alarm list\n";
    out << "  MCClock-CLI alarm add --time 07:30 --label \"Wake up\" --cycle daily\n";
    out << "  MCClock-CLI alarm list --json\n";
    out << "  MCClock-CLI birthday add --name \"Mom\" --date 1965-03-20\n";
    out << "  MCClock-CLI settings get reminder.volume\n";
    out << "  MCClock-CLI settings set reminder.volume 80\n";
    out << "  MCClock-CLI system info\n";
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationVersion("1.0.0");
    // Use the same org/app name as the GUI so both share the data directory
    // (%APPDATA%/MCClock/MCClock)
    app.setOrganizationName("MCClock");
    app.setApplicationName("MCClock");

    // Initialize database (shared with GUI; WAL + busy timeout)
    QString dbPath = PlatformUtils::databasePath();
    if (!mcclock::dal::Database::instance().initialize(dbPath)) {
        errStream << "Error: Failed to initialize database at: " << dbPath << "\n";
        return 1;
    }

    // Initialize settings
    SettingsManager::instance().load(PlatformUtils::settingsPath());

    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("MCClock CLI - Command line interface for MCClock alarm clock");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("module",
        "Module: alarm, birthday, shutdown, run, countdown, settings, system");
    parser.addPositionalArgument("action",
        "Action: list, add, edit, delete, ...");
    parser.addPositionalArgument("extra", "Extra arguments (settings get/set path and value)", "[extra...]");

    QCommandLineOption jsonOpt("json", "Output in JSON format");
    QCommandLineOption uuidOpt("uuid", "Item UUID", "uuid");
    QCommandLineOption labelOpt("label", "Item label", "label");
    QCommandLineOption timeOpt("time", "Time (HH:mm)", "time");
    QCommandLineOption cycleOpt("cycle", "Cycle mode: once,daily,weekly,monthly,yearly,interval", "cycle");
    QCommandLineOption cycleDataOpt("cycle-data", "Cycle parameters (JSON)", "data");
    QCommandLineOption nameOpt("name", "Birthday name", "name");
    QCommandLineOption dateOpt("date", "Date yyyy-MM-dd or MM-dd", "date");
    QCommandLineOption lunarOpt("lunar", "Treat --date as lunar month-day");
    QCommandLineOption remindTimeOpt("remind-time", "Birthday remind time (HH:mm)", "time");
    QCommandLineOption advanceOpt("advance", "Advance seconds/days", "n");
    QCommandLineOption optionOpt("option", "force-shutdown|shutdown|restart|logoff", "option");
    QCommandLineOption minutesOpt("minutes", "Countdown duration in minutes", "n");
    QCommandLineOption targetOpt("target", "Countdown target datetime (ISO8601)", "datetime");
    QCommandLineOption pathOpt("path", "Program path or URL", "path");
    QCommandLineOption argsOpt("args", "Program arguments", "args");
    QCommandLineOption fileOpt("file", "Export/import file path", "file");
    QCommandLineOption yesOpt("yes", "Confirm dangerous operations");

    parser.addOption(jsonOpt);
    parser.addOption(uuidOpt);
    parser.addOption(labelOpt);
    parser.addOption(timeOpt);
    parser.addOption(cycleOpt);
    parser.addOption(cycleDataOpt);
    parser.addOption(nameOpt);
    parser.addOption(dateOpt);
    parser.addOption(lunarOpt);
    parser.addOption(remindTimeOpt);
    parser.addOption(advanceOpt);
    parser.addOption(optionOpt);
    parser.addOption(minutesOpt);
    parser.addOption(targetOpt);
    parser.addOption(pathOpt);
    parser.addOption(argsOpt);
    parser.addOption(fileOpt);
    parser.addOption(yesOpt);

    parser.process(app);

    QStringList args = parser.positionalArguments();
    bool jsonOutput = parser.isSet(jsonOpt);

    if (args.isEmpty()) {
        printUsage();
        mcclock::dal::Database::instance().close();
        return 0;
    }

    QString module = args.value(0).toLower();
    QString action = args.value(1).toLower();
    int rc = 1;

    if (module == "alarm") {
        rc = handleAlarm(action, parser, uuidOpt, labelOpt, timeOpt, cycleOpt,
                         cycleDataOpt, fileOpt, jsonOutput);
    } else if (module == "birthday") {
        rc = handleBirthday(action, parser, uuidOpt, nameOpt, dateOpt, lunarOpt,
                            remindTimeOpt, advanceOpt, labelOpt, fileOpt, jsonOutput);
    } else if (module == "shutdown") {
        rc = handleShutdown(action, parser, uuidOpt, labelOpt, timeOpt, cycleOpt,
                            optionOpt, advanceOpt, yesOpt, fileOpt, jsonOutput);
    } else if (module == "run") {
        rc = handleRun(action, parser, uuidOpt, labelOpt, timeOpt, cycleOpt,
                       pathOpt, argsOpt, yesOpt, fileOpt, jsonOutput);
    } else if (module == "countdown") {
        rc = handleCountdown(action, parser, uuidOpt, labelOpt, minutesOpt, targetOpt,
                             fileOpt, jsonOutput);
    } else if (module == "settings") {
        rc = handleSettings(action, args, jsonOutput);
    } else if (module == "system") {
        rc = handleSystem(action, parser, yesOpt, optionOpt, fileOpt, jsonOutput);
    } else {
        errStream << "Error: unknown module: " << module << "\n\n";
        printUsage();
        rc = 1;
    }

    mcclock::dal::Database::instance().close();
    return rc;
}
