#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "main_window.h"
#include "theme_manager.h"
#include "core/dal/database.h"
#include "core/dal/settings_manager.h"
#include "core/utils/platform_utils.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MCClock");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MCClock");
    app.setQuitOnLastWindowClosed(false); // Keep running in tray

    QCommandLineParser parser;
    parser.setApplicationDescription("MCClock - \u68a6\u7545\u95f9\u949f"); // 梦畅闹钟
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption minimizedOpt("minimized", "Start minimized to system tray");
    parser.addOption(minimizedOpt);
    parser.process(app);

    // Initialize database
    QString dbPath = mcclock::utils::PlatformUtils::databasePath();
    if (!mcclock::dal::Database::instance().initialize(dbPath)) {
        qCritical() << "Failed to initialize database at:" << dbPath;
        return 1;
    }

    // Initialize settings
    QString settingsPath = mcclock::utils::PlatformUtils::settingsPath();
    mcclock::dal::SettingsManager::instance().load(settingsPath);

    // Apply flat theme
    mcclock::gui::ThemeManager::applyTheme(app);

    mcclock::gui::MainWindow window;
    if (parser.isSet(minimizedOpt)) {
        window.setStartMinimized(true);
    } else {
        window.show();
    }

    return app.exec();
}
