#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>

class QStackedWidget;

namespace mcclock::services {
class Scheduler;
class RingtoneManager;
}

namespace mcclock::gui {

class NavigationBar;

// Main application window (740x480): navigation bar + stacked pages.
// Pages are placeholders in P3; replaced by real feature pages in P4.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Show window restored from tray
    void showFromTray();

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
    QWidget* createPlaceholderPage(const QString& title);
    void saveClosePreference(int action, bool dontAskAgain);

    NavigationBar* navBar_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QSystemTrayIcon* trayIcon_ = nullptr;
    mcclock::services::Scheduler* scheduler_ = nullptr;
    mcclock::services::RingtoneManager* ringtone_ = nullptr;
    bool startMinimized_ = false;
    bool exitingFromTrayMenu_ = false;
};

} // namespace mcclock::gui
