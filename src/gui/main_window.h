#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>

class QStackedWidget;
class QAction;

namespace mcclock::services {
class Scheduler;
class RingtoneManager;
}

namespace mcclock::api {
class ApiServer;
}

namespace mcclock::gui {

class NavigationBar;
class SidebarWidget;
class DesktopClockWidget;

// Main application window (740x480): navigation bar + stacked pages.
// Pages are placeholders in P3; replaced by real feature pages in P4.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Show window restored from tray
    void showFromTray();

    // Toggle the desktop floating clock (also persists the setting)
    void setDesktopClockVisible(bool visible);
    bool isDesktopClockVisible() const;

    // Start minimized (boot auto-start mode)
    void setStartMinimized(bool v) { startMinimized_ = v; }

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void openSettings();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onAlarmTriggered();
    void onHourlyChime(int hour);

private:
    void setupUi();
    void setupTray();
    void setupScheduler();
    void checkMissedAlarms();
    void setupDesktopClock();
    void syncApiServer();
    QWidget* createPlaceholderPage(const QString& title);
    void saveClosePreference(int action, bool dontAskAgain);

    NavigationBar* navBar_ = nullptr;
    SidebarWidget* sidebar_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QSystemTrayIcon* trayIcon_ = nullptr;
    QAction* clockToggleAction_ = nullptr;
    QAction* noteToggleAction_ = nullptr;
    DesktopClockWidget* desktopClock_ = nullptr;
    mcclock::services::Scheduler* scheduler_ = nullptr;
    mcclock::services::RingtoneManager* ringtone_ = nullptr;
    mcclock::api::ApiServer* apiServer_ = nullptr;
    bool startMinimized_ = false;
    bool exitingFromTrayMenu_ = false;
};

} // namespace mcclock::gui
