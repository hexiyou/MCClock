#include "settings_dialog.h"
#include "core/dal/settings_manager.h"
#include "core/utils/platform_utils.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QTimeEdit>
#include <QSlider>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QMessageBox>
#include <QJsonArray>

namespace mcclock::gui {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("\u5168\u5c40\u8bbe\u7f6e")); // 全局设置
    resize(520, 460);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);

    auto* tabs = new QTabWidget(this);
    tabs->addTab(createGeneralTab(), QStringLiteral("\u57fa\u672c\u8bbe\u7f6e"));   // 基本设置
    tabs->addTab(createReminderTab(), QStringLiteral("\u63d0\u9192\u8bbe\u7f6e"));  // 提醒设置
    tabs->addTab(createChimeTab(), QStringLiteral("\u6574\u70b9\u62a5\u65f6"));     // 整点报时
    tabs->addTab(createAdvancedTab(), QStringLiteral("\u9ad8\u7ea7\u8bbe\u7f6e"));  // 高级设置
    root->addWidget(tabs, 1);

    auto* btnBar = new QHBoxLayout();
    btnBar->addStretch();
    auto* saveBtn = new QPushButton(QStringLiteral("\u4fdd\u5b58"), this);    // 保存
    auto* cancelBtn = new QPushButton(QStringLiteral("\u53d6\u6d88"), this);  // 取消
    cancelBtn->setProperty("flatStyle", "secondary");
    btnBar->addWidget(saveBtn);
    btnBar->addWidget(cancelBtn);
    root->addLayout(btnBar);

    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        saveSettings();
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    loadSettings();
}

QWidget* SettingsDialog::createGeneralTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    autoStartCheck_ = new QCheckBox(QStringLiteral("\u5f00\u673a\u81ea\u52a8\u542f\u52a8"), page); // 开机自动启动
    layout->addWidget(autoStartCheck_);

    missedCheck_ = new QCheckBox(QStringLiteral("\u542f\u52a8\u65f6\u63d0\u9192\u9057\u6f0f\u7684\u95f9\u949f"), page); // 启动时提醒遗漏的闹钟
    layout->addWidget(missedCheck_);

    autoUpdateCheck_ = new QCheckBox(QStringLiteral("\u542f\u52a8\u65f6\u81ea\u52a8\u68c0\u67e5\u66f4\u65b0"), page); // 启动时自动检查更新
    layout->addWidget(autoUpdateCheck_);

    desktopClockCheck_ = new QCheckBox(QStringLiteral("\u663e\u793a\u684c\u9762\u65f6\u949f"), page); // 显示桌面时钟
    layout->addWidget(desktopClockCheck_);

    auto* updateBtn = new QPushButton(QStringLiteral("\u7acb\u5373\u68c0\u67e5\u66f4\u65b0"), page); // 立即检查更新
    updateBtn->setFixedWidth(160);
    connect(updateBtn, &QPushButton::clicked, page, [page]() {
        QMessageBox::information(page, QStringLiteral("\u68c0\u67e5\u66f4\u65b0"),
            QStringLiteral("\u68c0\u67e5\u66f4\u65b0\u529f\u80fd\u6682\u672a\u4e0a\u7ebf")); // 检查更新功能暂未上线
    });
    layout->addWidget(updateBtn);

    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createReminderTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    // Fullscreen reminder mode
    auto* fsGroup = new QGroupBox(QStringLiteral("\u5168\u5c4f\u63d0\u9192\u6a21\u5f0f"), page); // 全屏提醒模式
    auto* fsLayout = new QVBoxLayout(fsGroup);
    fullscreenCheck_ = new QCheckBox(QStringLiteral("\u542f\u7528\u5168\u5c4f\u63d0\u9192\uff08\u6307\u5b9a\u65f6\u6bb5\u5185\uff09"), fsGroup); // 启用全屏提醒（指定时段内）
    fsLayout->addWidget(fullscreenCheck_);
    auto* timeRow = new QHBoxLayout();
    timeRow->addWidget(new QLabel(QStringLiteral("\u65f6\u6bb5\uff1a"), fsGroup)); // 时段：
    fullscreenStart_ = new QTimeEdit(fsGroup);
    fullscreenStart_->setDisplayFormat("HH:mm");
    fullscreenEnd_ = new QTimeEdit(fsGroup);
    fullscreenEnd_->setDisplayFormat("HH:mm");
    timeRow->addWidget(fullscreenStart_);
    timeRow->addWidget(new QLabel(QStringLiteral("\u81f3"), fsGroup)); // 至
    timeRow->addWidget(fullscreenEnd_);
    timeRow->addStretch();
    fsLayout->addLayout(timeRow);
    layout->addWidget(fsGroup);

    // Reminder popup position
    auto* posRow = new QHBoxLayout();
    posRow->addWidget(new QLabel(QStringLiteral("\u63d0\u9192\u7a97\u53e3\u4f4d\u7f6e\uff1a"), page)); // 提醒窗口位置：
    positionCombo_ = new QComboBox(page);
    positionCombo_->addItem(QStringLiteral("\u5c4f\u5e55\u5c45\u4e2d"), "center");       // 屏幕居中
    positionCombo_->addItem(QStringLiteral("\u5de6\u4e0a\u89d2"), "left_up");             // 左上角
    positionCombo_->addItem(QStringLiteral("\u5de6\u4e0b\u89d2"), "left_down");           // 左下角
    posRow->addWidget(positionCombo_);
    posRow->addStretch();
    layout->addLayout(posRow);

    // Reminder window close mode
    auto* closeRow = new QHBoxLayout();
    closeRow->addWidget(new QLabel(QStringLiteral("\u63d0\u9192\u7a97\u53e3\u5173\u95ed\u65b9\u5f0f\uff1a"), page)); // 提醒窗口关闭方式：
    closeModeCombo_ = new QComboBox(page);
    closeModeCombo_->addItem(QStringLiteral("\u624b\u52a8\u5173\u95ed"), "manual");        // 手动关闭
    closeModeCombo_->addItem(QStringLiteral("\u81ea\u52a8\u5173\u95ed"), "auto");          // 自动关闭
    closeRow->addWidget(closeModeCombo_);
    autoCloseSpin_ = new QSpinBox(page);
    autoCloseSpin_->setRange(1, 60);
    autoCloseSpin_->setSuffix(QStringLiteral(" \u5206\u949f")); // 分钟
    closeRow->addWidget(autoCloseSpin_);
    closeRow->addStretch();
    layout->addLayout(closeRow);

    // Volume
    auto* volRow = new QHBoxLayout();
    volRow->addWidget(new QLabel(QStringLiteral("\u94c3\u58f0\u97f3\u91cf\uff1a"), page)); // 铃声音量：
    volumeSlider_ = new QSlider(Qt::Horizontal, page);
    volumeSlider_->setRange(0, 100);
    volRow->addWidget(volumeSlider_, 1);
    auto* volLabel = new QLabel("100%", page);
    connect(volumeSlider_, &QSlider::valueChanged, volLabel, [volLabel](int v) {
        volLabel->setText(QString::number(v) + "%");
    });
    volRow->addWidget(volLabel);
    layout->addLayout(volRow);

    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createChimeTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    auto* modeRow = new QHBoxLayout();
    modeRow->addWidget(new QLabel(QStringLiteral("\u62a5\u65f6\u65b9\u5f0f\uff1a"), page)); // 报时方式：
    chimeModeCombo_ = new QComboBox(page);
    chimeModeCombo_->addItem(QStringLiteral("\u6587\u5b57+\u8bed\u97f3"), "text_and_voice"); // 文字+语音
    chimeModeCombo_->addItem(QStringLiteral("\u4ec5\u6587\u5b57"), "text");                   // 仅文字
    chimeModeCombo_->addItem(QStringLiteral("\u4ec5\u8bed\u97f3"), "voice");                  // 仅语音
    chimeModeCombo_->addItem(QStringLiteral("\u4e0d\u62a5\u65f6"), "off");                    // 不报时
    modeRow->addWidget(chimeModeCombo_);
    modeRow->addStretch();
    layout->addLayout(modeRow);

    auto* cycleRow = new QHBoxLayout();
    cycleRow->addWidget(new QLabel(QStringLiteral("\u62a5\u65f6\u5468\u671f\uff1a"), page)); // 报时周期：
    chimeCycleCombo_ = new QComboBox(page);
    chimeCycleCombo_->addItem(QStringLiteral("\u6bcf\u5c0f\u65f6"), "hourly");       // 每小时
    chimeCycleCombo_->addItem(QStringLiteral("\u6bcf\u534a\u5c0f\u65f6"), "half_hour"); // 每半小时
    chimeCycleCombo_->addItem(QStringLiteral("\u81ea\u5b9a\u4e49\u5c0f\u65f6"), "custom"); // 自定义小时
    cycleRow->addWidget(chimeCycleCombo_);
    cycleRow->addStretch();
    layout->addLayout(cycleRow);

    // Custom hours grid (0-23)
    auto* hoursGroup = new QGroupBox(QStringLiteral("\u81ea\u5b9a\u4e49\u62a5\u65f6\u5c0f\u65f6"), page); // 自定义报时小时
    auto* grid = new QGridLayout(hoursGroup);
    for (int h = 0; h < 24; ++h) {
        auto* check = new QCheckBox(QStringLiteral("%1\u70b9").arg(h), hoursGroup); // X点
        grid->addWidget(check, h / 6, h % 6);
        chimeHourChecks_.append(check);
    }
    layout->addWidget(hoursGroup);

    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createAdvancedTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    // HTTP API server
    auto* apiGroup = new QGroupBox(QStringLiteral("HTTP API \u670d\u52a1\u5668"), page); // HTTP API 服务器
    auto* apiLayout = new QFormLayout(apiGroup);
    apiEnabledCheck_ = new QCheckBox(QStringLiteral("\u542f\u7528 HTTP API \u670d\u52a1\uff08\u9ed8\u8ba4\u4e0d\u542f\u7528\uff09"), apiGroup); // 启用 HTTP API 服务（默认不启用）
    apiLayout->addRow(apiEnabledCheck_);
    apiIpEdit_ = new QLineEdit("127.0.0.1", apiGroup);
    apiLayout->addRow(QStringLiteral("\u7ed1\u5b9a IP\uff1a"), apiIpEdit_); // 绑定 IP：
    apiPortSpin_ = new QSpinBox(apiGroup);
    apiPortSpin_->setRange(1, 65535);
    apiPortSpin_->setValue(8900);
    apiLayout->addRow(QStringLiteral("\u76d1\u542c\u7aef\u53e3\uff1a"), apiPortSpin_); // 监听端口：
    layout->addWidget(apiGroup);

    // Account (cloud sync stub)
    auto* accountGroup = new QGroupBox(QStringLiteral("\u8d26\u53f7\uff08\u4e91\u540c\u6b65\uff09"), page); // 账号（云同步）
    auto* accountLayout = new QHBoxLayout(accountGroup);
    auto* loginBtn = new QPushButton(QStringLiteral("\u767b\u5f55"), accountGroup);   // 登录
    auto* registerBtn = new QPushButton(QStringLiteral("\u6ce8\u518c"), accountGroup); // 注册
    registerBtn->setProperty("flatStyle", "secondary");
    accountLayout->addWidget(loginBtn);
    accountLayout->addWidget(registerBtn);
    accountLayout->addStretch();
    connect(loginBtn, &QPushButton::clicked, page, [page]() {
        QMessageBox::information(page, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u529f\u80fd\u6682\u672a\u4e0a\u7ebf")); // 功能暂未上线
    });
    connect(registerBtn, &QPushButton::clicked, page, [page]() {
        QMessageBox::information(page, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u529f\u80fd\u6682\u672a\u4e0a\u7ebf")); // 功能暂未上线
    });
    layout->addWidget(accountGroup);

    layout->addStretch();
    return page;
}

void SettingsDialog::loadSettings() {
    auto& s = mcclock::dal::SettingsManager::instance();

    autoStartCheck_->setChecked(s.autoStart());
    missedCheck_->setChecked(s.missedReminder());
    autoUpdateCheck_->setChecked(s.autoCheckUpdate());
    desktopClockCheck_->setChecked(s.desktopClock());

    fullscreenCheck_->setChecked(s.fullscreenMode());
    fullscreenStart_->setTime(QTime::fromString(s.fullscreenTimeStart(), "HH:mm"));
    fullscreenEnd_->setTime(QTime::fromString(s.fullscreenTimeEnd(), "HH:mm"));
    positionCombo_->setCurrentIndex(positionCombo_->findData(s.reminderPosition()));
    closeModeCombo_->setCurrentIndex(closeModeCombo_->findData(s.closeMode()));
    autoCloseSpin_->setValue(s.autoCloseMinutes());
    volumeSlider_->setValue(s.alarmVolume());

    chimeModeCombo_->setCurrentIndex(chimeModeCombo_->findData(s.chimeMode()));
    chimeCycleCombo_->setCurrentIndex(chimeCycleCombo_->findData(s.chimeCycle()));
    const QJsonArray hours = s.chimeHours();
    for (int h = 0; h < chimeHourChecks_.size() && h < 24; ++h) {
        chimeHourChecks_[h]->setChecked(hours.contains(QJsonValue(h)));
    }

    apiEnabledCheck_->setChecked(s.httpApiEnabled());
    apiIpEdit_->setText(s.httpApiBindIp());
    apiPortSpin_->setValue(s.httpApiPort());
}

void SettingsDialog::saveSettings() {
    auto& s = mcclock::dal::SettingsManager::instance();

    bool autoStart = autoStartCheck_->isChecked();
    s.setAutoStart(autoStart);
    mcclock::utils::PlatformUtils::setAutoStart(autoStart);
    s.setMissedReminder(missedCheck_->isChecked());
    s.setAutoCheckUpdate(autoUpdateCheck_->isChecked());
    s.setDesktopClock(desktopClockCheck_->isChecked());

    s.setFullscreenMode(fullscreenCheck_->isChecked());
    s.setFullscreenTimeStart(fullscreenStart_->time().toString("HH:mm"));
    s.setFullscreenTimeEnd(fullscreenEnd_->time().toString("HH:mm"));
    s.setReminderPosition(positionCombo_->currentData().toString());
    s.setCloseMode(closeModeCombo_->currentData().toString());
    s.setAutoCloseMinutes(autoCloseSpin_->value());
    s.setAlarmVolume(volumeSlider_->value());

    s.setChimeMode(chimeModeCombo_->currentData().toString());
    s.setChimeCycle(chimeCycleCombo_->currentData().toString());
    QJsonArray hours;
    for (int h = 0; h < chimeHourChecks_.size() && h < 24; ++h) {
        if (chimeHourChecks_[h]->isChecked()) hours.append(h);
    }
    s.setChimeHours(hours);

    s.setHttpApiEnabled(apiEnabledCheck_->isChecked());
    s.setHttpApiBindIp(apiIpEdit_->text());
    s.setHttpApiPort(apiPortSpin_->value());

    s.save();
    emit settingsSaved();
}

} // namespace mcclock::gui
