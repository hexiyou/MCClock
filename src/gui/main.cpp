#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QDialogButtonBox>

#include "main_window.h"
#include "theme_manager.h"
#include "core/dal/database.h"
#include "core/dal/settings_manager.h"
#include "core/utils/platform_utils.h"

// Global event filter to translate dialog button texts to Chinese
class DialogButtonTranslator : public QObject {
public:
    using QObject::QObject;
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Show) {
            // Handle QMessageBox
            auto* msgBox = qobject_cast<QMessageBox*>(obj);
            if (msgBox) {
                if (auto* btn = msgBox->button(QMessageBox::Ok))
                    btn->setText(QStringLiteral("\u786e\u5b9a")); // \u786e\u5b9a
                if (auto* btn = msgBox->button(QMessageBox::Cancel))
                    btn->setText(QStringLiteral("\u53d6\u6d88")); // \u53d6\u6d88
                if (auto* btn = msgBox->button(QMessageBox::Yes))
                    btn->setText(QStringLiteral("\u662f")); // \u662f
                if (auto* btn = msgBox->button(QMessageBox::No))
                    btn->setText(QStringLiteral("\u5426")); // \u5426
                return false;
            }
            // Handle QDialog (QInputDialog, etc.)
            auto* dlg = qobject_cast<QDialog*>(obj);
            if (dlg && !msgBox) {
                // Find OK/Cancel buttons in the dialog
                auto buttons = dlg->findChildren<QPushButton*>();
                for (auto* btn : buttons) {
                    QString text = btn->text().trimmed();
                    if (text == "OK" || text == "&OK")
                        btn->setText(QStringLiteral("\u786e\u5b9a")); // \u786e\u5b9a
                    else if (text == "Cancel" || text == "&Cancel")
                        btn->setText(QStringLiteral("\u53d6\u6d88")); // \u53d6\u6d88
                    else if (text == "Yes" || text == "&Yes")
                        btn->setText(QStringLiteral("\u662f")); // \u662f
                    else if (text == "No" || text == "&No")
                        btn->setText(QStringLiteral("\u5426")); // \u5426
                }
                // Also check QDialogButtonBox
                auto* box = dlg->findChild<QDialogButtonBox*>();
                if (box) {
                    if (auto* btn = box->button(QDialogButtonBox::Ok))
                        btn->setText(QStringLiteral("\u786e\u5b9a")); // \u786e\u5b9a
                    if (auto* btn = box->button(QDialogButtonBox::Cancel))
                        btn->setText(QStringLiteral("\u53d6\u6d88")); // \u53d6\u6d88
                    if (auto* btn = box->button(QDialogButtonBox::Yes))
                        btn->setText(QStringLiteral("\u662f")); // \u662f
                    if (auto* btn = box->button(QDialogButtonBox::No))
                        btn->setText(QStringLiteral("\u5426")); // \u5426
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

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

    // Sync auto-start state with registry on startup
    bool autoStart = mcclock::dal::SettingsManager::instance().autoStart();
    mcclock::utils::PlatformUtils::setAutoStart(autoStart);

    // Apply theme with saved color
    auto& settings = mcclock::dal::SettingsManager::instance();
    QJsonValue themeColorValue = settings.get({"ui", "theme_color"});
    QString themeColor = themeColorValue.isString() ? themeColorValue.toString() : "#1E88E5";
    QColor primaryColor(themeColor);
    mcclock::gui::ThemeManager::applyTheme(app, primaryColor);

    // Install global translator for dialog button texts
    app.installEventFilter(new DialogButtonTranslator(&app));

    mcclock::gui::MainWindow window;
    if (parser.isSet(minimizedOpt)) {
        window.setStartMinimized(true);
    } else {
        window.show();
    }

    return app.exec();
}
