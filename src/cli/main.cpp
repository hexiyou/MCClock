#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTextStream>
#include <QDebug>

#include "core/dal/database.h"
#include "core/dal/settings_manager.h"
#include "core/utils/platform_utils.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("MCClock-CLI");
    app.setApplicationVersion("1.0.0");

    QTextStream out(stdout);
    QTextStream err(stderr);

    // Initialize database
    QString dbPath = mcclock::utils::PlatformUtils::databasePath();
    if (!mcclock::dal::Database::instance().initialize(dbPath)) {
        err << "Error: Failed to initialize database at: " << dbPath << "\n";
        return 1;
    }

    // Initialize settings
    QString settingsPath = mcclock::utils::PlatformUtils::settingsPath();
    mcclock::dal::SettingsManager::instance().load(settingsPath);

    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("MCClock CLI - Command line interface for MCClock alarm clock");
    parser.addHelpOption();
    parser.addVersionOption();

    // Module argument
    parser.addPositionalArgument("module",
        "Module: alarm, birthday, shutdown, run, countdown, settings, system");
    parser.addPositionalArgument("action",
        "Action: list, add, edit, delete, restore, export, import, get, set");

    // Common options
    QCommandLineOption jsonOpt("json", "Output in JSON format");
    QCommandLineOption quietOpt("quiet", "Minimal output");
    QCommandLineOption uuidOpt("uuid", "Task UUID", "uuid");
    QCommandLineOption labelOpt("label", "Task label", "label");
    QCommandLineOption timeOpt("time", "Time (HH:mm)", "time");
    QCommandLineOption cycleOpt("cycle", "Cycle mode: once,daily,weekly,monthly,yearly,interval", "cycle");

    parser.addOption(jsonOpt);
    parser.addOption(quietOpt);
    parser.addOption(uuidOpt);
    parser.addOption(labelOpt);
    parser.addOption(timeOpt);
    parser.addOption(cycleOpt);

    parser.process(app);

    QStringList args = parser.positionalArguments();
    bool jsonOutput = parser.isSet(jsonOpt);

    if (args.isEmpty()) {
        out << "MCClock CLI v1.0.0\n\n";
        out << "Usage: MCClock-CLI.exe <module> <action> [options]\n\n";
        out << "Modules:\n";
        out << "  alarm       Alarm management\n";
        out << "  birthday    Birthday management\n";
        out << "  shutdown    Scheduled shutdown\n";
        out << "  run         Run program tasks\n";
        out << "  countdown   Countdown timers\n";
        out << "  settings    Application settings\n";
        out << "  system      System operations\n\n";
        out << "Actions:\n";
        out << "  list        List all items\n";
        out << "  add         Add new item\n";
        out << "  edit        Edit item (requires --uuid)\n";
        out << "  delete      Delete item (requires --uuid)\n";
        out << "  restore     Restore deleted item\n";
        out << "  export      Export data\n";
        out << "  import      Import data\n\n";
        out << "Options:\n";
        out << "  --json      JSON output format\n";
        out << "  --uuid      Task UUID\n";
        out << "  --label     Task label\n";
        out << "  --time      Time (HH:mm)\n";
        out << "  --cycle     Cycle mode\n\n";
        out << "Examples:\n";
        out << "  MCClock-CLI alarm list\n";
        out << "  MCClock-CLI alarm add --time 07:30 --label \"Wake up\" --cycle daily\n";
        out << "  MCClock-CLI alarm delete --uuid xxx\n";
        out << "  MCClock-CLI settings get alarm.volume\n";
        out << "  MCClock-CLI settings set alarm.volume 80\n";

        mcclock::dal::Database::instance().close();
        return 0;
    }

    QString module = args.value(0).toLower();
    QString action = args.value(1).toLower();

    // TODO: Implement command handlers in P7
    // For now, just acknowledge the command
    out << "Module: " << module << ", Action: " << action << "\n";
    out << "CLI commands will be fully implemented in P7.\n";

    mcclock::dal::Database::instance().close();
    return 0;
}
