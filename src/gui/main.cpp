#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>

#include "core/dal/database.h"
#include "core/dal/settings_manager.h"
#include "core/utils/platform_utils.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MCClock");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MCClock");

    // Initialize database
    QString dbPath = mcclock::utils::PlatformUtils::databasePath();
    if (!mcclock::dal::Database::instance().initialize(dbPath)) {
        qCritical() << "Failed to initialize database at:" << dbPath;
        return 1;
    }

    // Initialize settings
    QString settingsPath = mcclock::utils::PlatformUtils::settingsPath();
    mcclock::dal::SettingsManager::instance().load(settingsPath);

    qDebug() << "MCClock GUI starting...";
    qDebug() << "Database:" << dbPath;
    qDebug() << "Settings:" << settingsPath;

    // Temporary placeholder window (will be replaced by MainWindow in P3)
    QMainWindow window;
    window.setWindowTitle(QStringLiteral("\u68a6\u7545\u95f9\u949f v1.0.0"));
    window.resize(740, 480);

    auto* centralWidget = new QWidget(&window);
    auto* layout = new QVBoxLayout(centralWidget);
    auto* label = new QLabel(QStringLiteral("MCClock - \u68a6\u7545\u95f9\u949f\n"
        "\u9879\u76ee\u9aa8\u67b6\u5df2\u5c31\u7eea\n"
        "GUI \u754c\u9762\u5c06\u5728 P3 \u9636\u6bb5\u5b9e\u73b0"),
        centralWidget);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 18px; color: #1E88E5;");
    layout->addWidget(label);
    window.setCentralWidget(centralWidget);

    window.show();

    int result = app.exec();

    // Cleanup
    mcclock::dal::Database::instance().close();

    return result;
}
