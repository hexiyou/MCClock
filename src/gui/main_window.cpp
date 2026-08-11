#include "main_window.h"
#include "widgets/navigation_bar.h"
#include "widgets/reminder_popup.h"
#include "widgets/sidebar_widget.h"
#include "widgets/desktop_clock_widget.h"
#include "dialogs/close_confirm_dialog.h"
#include "dialogs/settings_dialog.h"
#include "pages/home_page.h"
#include "pages/alarm_page.h"
#include "pages/birthday_page.h"
#include "pages/task_pages.h"
#include "pages/countdown_page.h"
#include "pages/stopwatch_page.h"
#include "pages/health_page.h"
#include "dialogs/missed_reminder_dialog.h"
#include "theme_manager.h"
#include "core/services/scheduler.h"
#include "core/services/ringtone_manager.h"
#include "core/services/business_services.h"
#include "core/dal/settings_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>

namespace mcclock::gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("\u68a6\u7545\u95f9\u949f")); // 梦畅闹钟
    setWindowIcon(ThemeManager::appIcon());
    setFixedSize(740, 480);

    setupUi();
    setupTray();
    setupScheduler();
    setupDesktopClock();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    navBar_ = new NavigationBar(central);
    root->addWidget(navBar_);

    pages_ = new QStackedWidget(central);
    {
        pages_->addWidget(new HomePage(this));

        auto* alarmPage = new AlarmPage(this);
        connect(alarmPage, &AlarmPage::dataChanged, scheduler_,
                &mcclock::services::Scheduler::reload);
        pages_->addWidget(alarmPage);

        auto* birthdayPage = new BirthdayPage(this);
        connect(birthdayPage, &BirthdayPage::dataChanged, scheduler_,
                &mcclock::services::Scheduler::reload);
        pages_->addWidget(birthdayPage);

        auto* shutdownPage = new ShutdownPage(this);
        connect(shutdownPage, &ShutdownPage::dataChanged, scheduler_,
                &mcclock::services::Scheduler::reload);
        pages_->addWidget(shutdownPage);

        auto* runProgramPage = new RunProgramPage(this);
        connect(runProgramPage, &RunProgramPage::dataChanged, scheduler_,
                &mcclock::services::Scheduler::reload);
        pages_->addWidget(runProgramPage);

        pages_->addWidget(new CountdownPage(this));
        pages_->addWidget(new StopwatchPage(this));
        pages_->addWidget(new HealthPage(this));
    }

    sidebar_ = new SidebarWidget(central);
    connect(sidebar_, &SidebarWidget::desktopClockToggled,
            this, &MainWindow::setDesktopClockVisible);

    auto* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);
    body->addWidget(pages_, 1);
    body->addWidget(sidebar_);
    root->addLayout(body, 1);

    setCentralWidget(central);

    connect(navBar_, &NavigationBar::currentIndexChanged,
            pages_, &QStackedWidget::setCurrentIndex);
    connect(navBar_, &NavigationBar::settingsClicked, this, &MainWindow::openSettings);
    connect(navBar_, &NavigationBar::skinClicked, this, [this]() {
        QMessageBox::information(this, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u76ae\u80a4\u5207\u6362\u529f\u80fd\u5f00\u53d1\u4e2d")); // 皮肤切换功能开发中
    });
}

QWidget* MainWindow::createPlaceholderPage(const QString& title) {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* label = new QLabel(title + QStringLiteral(" - \u9875\u9762\u5f00\u53d1\u4e2d"), page); // 页面开发中
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 20px; color: #90A4AE;");
    layout->addWidget(label);
    return page;
}

void MainWindow::setupTray() {
    trayIcon_ = new QSystemTrayIcon(ThemeManager::appIcon(), this);
    trayIcon_->setToolTip(QStringLiteral("\u68a6\u7545\u95f9\u949f")); // 梦畅闹钟

    auto* menu = new QMenu(this);
    auto* showAction = menu->addAction(QStringLiteral("\u663e\u793a\u4e3b\u754c\u9762")); // 显示主界面
    connect(showAction, &QAction::triggered, this, &MainWindow::showFromTray);
    menu->addSeparator();
    auto* exitAction = menu->addAction(QStringLiteral("\u9000\u51fa")); // 退出
    connect(exitAction, &QAction::triggered, this, [this]() {
        exitingFromTrayMenu_ = true;
        close();
        QApplication::quit();
    });

    trayIcon_->setContextMenu(menu);
    connect(trayIcon_, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
    trayIcon_->show();
}

void MainWindow::setupScheduler() {
    scheduler_ = new mcclock::services::Scheduler(this);
    ringtone_ = new mcclock::services::RingtoneManager(this);

    connect(scheduler_, &mcclock::services::Scheduler::alarmTriggered,
            this, [this](const mcclock::models::Alarm& alarm) {
        auto& settings = mcclock::dal::SettingsManager::instance();
        ringtone_->play(alarm.ringtone, alarm.customRingtonePath, alarm.ringMode,
                        alarm.customMinutes, settings.alarmVolume());

        QString msg = QStringLiteral("\u73b0\u5728\u662f %1").arg(alarm.time); // 现在是 HH:mm
        if (!alarm.label.isEmpty()) {
            msg += QStringLiteral("\uff0c") + alarm.label; // ，备注
        }
        auto* popup = new ReminderPopup(
            QStringLiteral("\u95f9\u949f\u63d0\u9192"), msg, nullptr); // 闹钟提醒
        connect(popup, &ReminderPopup::dismissClicked, this, [this]() {
            ringtone_->stop();
        });
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->showAtConfiguredPosition();
    });

    connect(scheduler_, &mcclock::services::Scheduler::birthdayTriggered,
            this, [this](const mcclock::models::Birthday& b) {
        auto& settings = mcclock::dal::SettingsManager::instance();
        ringtone_->play(b.ringtone, b.customRingtonePath, b.ringMode,
                        b.customMinutes, settings.alarmVolume());
        auto* popup = new ReminderPopup(
            QStringLiteral("\u751f\u65e5\u63d0\u9192"), // 生日提醒
            QStringLiteral("%1 \u7684\u751f\u65e5\u5373\u5c06\u5230\u6765\uff0c\u4e0d\u8981\u5fd8\u8bb0\u9001\u4e0a\u795d\u798f\uff01").arg(b.name),
            nullptr);
        connect(popup, &ReminderPopup::dismissClicked, this, [this]() {
            ringtone_->stop();
        });
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->showAtConfiguredPosition();
    });

    connect(scheduler_, &mcclock::services::Scheduler::hourlyChime,
            this, &MainWindow::onHourlyChime);

    connect(scheduler_, &mcclock::services::Scheduler::shutdownWarning,
            this, [this](const mcclock::models::ShutdownTask& task, int secondsLeft) {
        trayIcon_->showMessage(QStringLiteral("\u5b9a\u65f6\u5173\u673a\u9884\u8b66"), // 定时关机预警
            QStringLiteral("%1 \u5c06\u5728 %2 \u79d2\u540e\u6267\u884c").arg(task.time).arg(secondsLeft), // HH:mm 将在 N 秒后执行
            QSystemTrayIcon::Warning, 3000);
    });

    connect(scheduler_, &mcclock::services::Scheduler::shutdownDue,
            this, [](const mcclock::models::ShutdownTask& task) {
        mcclock::services::ShutdownService().executeNow(task);
    });

    connect(scheduler_, &mcclock::services::Scheduler::runProgramTriggered,
            this, [](const mcclock::models::RunProgramTask& task) {
        mcclock::services::RunProgramService().executeNow(task);
    });

    scheduler_->start();

    checkMissedAlarms();
}

void MainWindow::checkMissedAlarms() {
    auto& s = mcclock::dal::SettingsManager::instance();
    if (!s.missedReminder()) return;

    QString lastRun = s.get({"app", "last_run_time"}).toString();
    QDateTime lastRunDt = QDateTime::fromString(lastRun, Qt::ISODate);
    if (lastRunDt.isValid()) {
        auto missed = mcclock::services::AlarmService().findMissed(lastRunDt);
        if (!missed.isEmpty()) {
            MissedReminderDialog dlg(missed, this);
            dlg.exec();
        }
    }
    s.set({"app", "last_run_time"}, QDateTime::currentDateTime().toString(Qt::ISODate));
    s.save();
}

void MainWindow::showFromTray() {
    showNormal();
    activateWindow();
    raise();
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        if (isVisible() && !isMinimized()) {
            hide();
        } else {
            showFromTray();
        }
    }
}

void MainWindow::onAlarmTriggered() {
    // Handled in the lambda above
}

void MainWindow::onHourlyChime(int hour) {
    // Floating popup at top of screen
    new HourlyChimePopup(hour, nullptr);
    trayIcon_->showMessage(QStringLiteral("\u6574\u70b9\u62a5\u65f6"), // 整点报时
        QStringLiteral("\u73b0\u5728\u662f %1 \u70b9\u6574").arg(hour),
        QSystemTrayIcon::Information, 3000);
}

void MainWindow::setupDesktopClock() {
    auto& s = mcclock::dal::SettingsManager::instance();
    if (s.desktopClock()) {
        setDesktopClockVisible(true);
    }
}

void MainWindow::setDesktopClockVisible(bool visible) {
    auto& s = mcclock::dal::SettingsManager::instance();
    if (visible) {
        if (!desktopClock_) {
            desktopClock_ = new DesktopClockWidget(nullptr);
        }
        desktopClock_->show();
    } else if (desktopClock_) {
        desktopClock_->hide();
    }
    s.setDesktopClock(visible);
    s.save();
}

void MainWindow::openSettings() {
    SettingsDialog dlg(this);
    connect(&dlg, &SettingsDialog::settingsSaved, this, [this]() {
        // P6: restart HTTP API server here if settings changed
        Q_UNUSED(this);
    });
    dlg.exec();
}

void MainWindow::saveClosePreference(int action, bool dontAskAgain) {
    auto& s = mcclock::dal::SettingsManager::instance();
    s.set({"ui", "close_action"}, action);
    s.set({"ui", "show_close_confirm"}, !dontAskAgain);
    s.save();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (exitingFromTrayMenu_) {
        event->accept();
        return;
    }

    auto& s = mcclock::dal::SettingsManager::instance();
    bool showConfirm = s.get({"ui", "show_close_confirm"}).toBool(true);

    if (!showConfirm) {
        int action = s.get({"ui", "close_action"}).toInt(0);
        if (action == CloseConfirmDialog::ExitProgram) {
            event->accept();
        } else {
            hide();
            event->ignore();
        }
        return;
    }

    CloseConfirmDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        saveClosePreference(dlg.selectedAction(), dlg.dontAskAgain());
        if (dlg.selectedAction() == CloseConfirmDialog::ExitProgram) {
            event->accept();
        } else {
            hide();
            event->ignore();
        }
    } else {
        event->ignore();
    }
}

} // namespace mcclock::gui
