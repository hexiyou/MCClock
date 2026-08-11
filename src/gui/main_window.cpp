#include "main_window.h"
#include "widgets/navigation_bar.h"
#include "dialogs/close_confirm_dialog.h"
#include "theme_manager.h"
#include "core/services/scheduler.h"
#include "core/services/ringtone_manager.h"
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
    const QStringList pageTitles = {
        QStringLiteral("\u9996\u9875"),             // 首页
        QStringLiteral("\u95f9\u949f\u63d0\u9192"), // 闹钟提醒
        QStringLiteral("\u751f\u65e5\u63d0\u9192"), // 生日提醒
        QStringLiteral("\u5b9a\u65f6\u5173\u673a"), // 定时关机
        QStringLiteral("\u8fd0\u884c\u7a0b\u5e8f"), // 运行程序
        QStringLiteral("\u5012\u8ba1\u65f6"),       // 倒计时
        QStringLiteral("\u8ba1\u65f6\u5668"),       // 计时器
        QStringLiteral("\u5065\u5eb7\u63d0\u9192")  // 健康提醒
    };
    for (const auto& t : pageTitles) {
        pages_->addWidget(createPlaceholderPage(t));
    }
    root->addWidget(pages_, 1);

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
        // P4: show ReminderPopup; for now play ringtone + tray message
        auto& settings = mcclock::dal::SettingsManager::instance();
        ringtone_->play(alarm.ringtone, alarm.customRingtonePath, alarm.ringMode,
                        alarm.customMinutes, settings.alarmVolume());
        trayIcon_->showMessage(QStringLiteral("\u95f9\u949f\u63d0\u9192"), // 闹钟提醒
            alarm.label.isEmpty() ? alarm.time : alarm.label,
            QSystemTrayIcon::Information, 10000);
    });

    connect(scheduler_, &mcclock::services::Scheduler::birthdayTriggered,
            this, [this](const mcclock::models::Birthday& b) {
        trayIcon_->showMessage(QStringLiteral("\u751f\u65e5\u63d0\u9192"), // 生日提醒
            QStringLiteral("\u4eca\u5929\u662f %1 \u7684\u751f\u65e5\uff01").arg(b.name),
            QSystemTrayIcon::Information, 10000);
    });

    connect(scheduler_, &mcclock::services::Scheduler::hourlyChime,
            this, &MainWindow::onHourlyChime);

    scheduler_->start();
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
    // P5: HourlyChimePopup floating window; for now tray message
    Q_UNUSED(hour);
    trayIcon_->showMessage(QStringLiteral("\u6574\u70b9\u62a5\u65f6"), // 整点报时
        QStringLiteral("\u73b0\u5728\u662f %1 \u70b9\u6574").arg(hour),
        QSystemTrayIcon::Information, 5000);
}

void MainWindow::openSettings() {
    // P4: SettingsDialog
    QMessageBox::information(this, QStringLiteral("\u5168\u5c40\u8bbe\u7f6e"),
        QStringLiteral("\u8bbe\u7f6e\u9762\u677f\u5f00\u53d1\u4e2d")); // 设置面板开发中
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
