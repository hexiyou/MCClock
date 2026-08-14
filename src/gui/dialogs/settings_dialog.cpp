#include "settings_dialog.h"
#include "../theme_manager.h"
#include "core/dal/settings_manager.h"
#include "core/utils/platform_utils.h"
#include "core/services/business_services.h"

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
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QRandomGenerator>
#include <QPainter>

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
    tabs->addTab(createAboutTab(), QStringLiteral("\u5173\u4e8e"));                 // 关于
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
    chimeCycleCombo_->addItem(QStringLiteral("\u6bcf\u5c0f\u65f6"), "hourly");       // \u6bcf\u5c0f\u65f6
    chimeCycleCombo_->addItem(QStringLiteral("\u6bcf\u534a\u5c0f\u65f6"), "half_hour"); // \u6bcf\u534a\u5c0f\u65f6
    chimeCycleCombo_->addItem(QStringLiteral("\u81ea\u5b9a\u4e49\u5206\u949f"), "custom"); // \u81ea\u5b9a\u4e49\u5206\u949f
    cycleRow->addWidget(chimeCycleCombo_);
    cycleRow->addStretch();
    layout->addLayout(cycleRow);
    
    // Custom minute selector
    auto* minuteRow = new QHBoxLayout();
    minuteRow->addWidget(new QLabel(QStringLiteral("\u62a5\u65f6\u5206\u6570\uff1a"), page)); // \u62a5\u65f6\u5206\u6570\uff1a
    chimeMinuteCombo_ = new QComboBox(page);
    chimeMinuteCombo_->addItem(QStringLiteral("\u6bcf\u5c0f\u65f6\u6574\u70b9"), -1); // \u6bcf\u5c0f\u65f6\u6574\u70b9
    for (int m : {1, 2, 3, 4, 5, 6, 10, 12, 15, 20}) {
        chimeMinuteCombo_->addItem(QStringLiteral("%1 \u5206").arg(m), m); // X \u5206
    }
    minuteRow->addWidget(chimeMinuteCombo_);
    minuteRow->addStretch();
    layout->addLayout(minuteRow);
    
    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createAdvancedTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    // Data import/export
    auto* dataGroup = new QGroupBox(QStringLiteral("\u6570\u636e\u5bfc\u5165\u5bfc\u51fa"), page); // 数据导入导出
    auto* dataLayout = new QHBoxLayout(dataGroup);
    auto* exportBtn = new QPushButton(QStringLiteral("\u5bfc\u51fa\u6570\u636e"), dataGroup);   // 导出数据
    auto* importBtn = new QPushButton(QStringLiteral("\u5bfc\u5165\u6570\u636e"), dataGroup);   // 导入数据
    importBtn->setProperty("flatStyle", "secondary");
    dataLayout->addWidget(exportBtn);
    dataLayout->addWidget(importBtn);
    dataLayout->addStretch();

    connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::exportAllData);
    connect(importBtn, &QPushButton::clicked, this, &SettingsDialog::importAllData);
    layout->addWidget(dataGroup);

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

    // Read auto-start from registry (authoritative source)
    autoStartCheck_->setChecked(mcclock::utils::PlatformUtils::isAutoStartEnabled());
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
    chimeMinuteCombo_->setCurrentIndex(chimeMinuteCombo_->findData(s.chimeMinute()));

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
    s.setChimeMinute(chimeMinuteCombo_->currentData().toInt());

    s.setHttpApiEnabled(apiEnabledCheck_->isChecked());
    s.setHttpApiBindIp(apiIpEdit_->text());
    s.setHttpApiPort(apiPortSpin_->value());

    s.save();
    emit settingsSaved();
}

void SettingsDialog::exportAllData() {
    QString filePath = QFileDialog::getSaveFileName(this,
        QStringLiteral("\u5bfc\u51fa\u6570\u636e"), // 导出数据
        QStringLiteral("mcclock-backup.json"),
        QStringLiteral("JSON (*.json)"));

    if (filePath.isEmpty()) return;

    using namespace mcclock::services;

    QJsonObject root;

    // Export alarms
    QJsonArray alarms;
    for (const auto& a : AlarmService().findAll()) {
        alarms.append(a.toJson());
    }
    root["alarms"] = alarms;

    // Export birthdays
    QJsonArray birthdays;
    for (const auto& b : BirthdayService().findAll()) {
        birthdays.append(b.toJson());
    }
    root["birthdays"] = birthdays;

    // Export shutdown tasks
    QJsonArray shutdownTasks;
    for (const auto& t : ShutdownService().findAll()) {
        shutdownTasks.append(t.toJson());
    }
    root["shutdown_tasks"] = shutdownTasks;

    // Export run program tasks
    QJsonArray runPrograms;
    for (const auto& t : RunProgramService().findAll()) {
        runPrograms.append(t.toJson());
    }
    root["run_programs"] = runPrograms;

    // Export countdowns
    QJsonArray countdowns;
    for (const auto& c : CountdownService().findAll()) {
        countdowns.append(c.toJson());
    }
    root["countdowns"] = countdowns;

    // Export settings
    root["settings"] = mcclock::dal::SettingsManager::instance().rootObject();

    // Export sticky notes
    QString stickyPath = mcclock::utils::PlatformUtils::appDataPath() + "/sticky_notes.json";
    if (QFile::exists(stickyPath)) {
        QFile sf(stickyPath);
        if (sf.open(QIODevice::ReadOnly)) {
            QJsonObject stickyObj = QJsonDocument::fromJson(sf.readAll()).object();
            root["sticky_notes"] = stickyObj;
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("\u9519\u8bef"), // 错误
            QStringLiteral("\u65e0\u6cd5\u5199\u5165\u6587\u4ef6")); // 无法写入文件
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, QStringLiteral("\u6210\u529f"), // 成功
        QStringLiteral("\u6570\u636e\u5df2\u5bfc\u51fa\u5230\uff1a%1").arg(filePath)); // 数据已导出到：
}

void SettingsDialog::importAllData() {
    QString filePath = QFileDialog::getOpenFileName(this,
        QStringLiteral("\u5bfc\u5165\u6570\u636e"), // 导入数据
        QString(),
        QStringLiteral("JSON (*.json)"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("\u9519\u8bef"), // 错误
            QStringLiteral("\u65e0\u6cd5\u8bfb\u53d6\u6587\u4ef6")); // 无法读取文件
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, QStringLiteral("\u9519\u8bef"), // 错误
            QStringLiteral("\u6587\u4ef6\u683c\u5f0f\u65e0\u6548")); // 文件格式无效
        return;
    }

    QJsonObject root = doc.object();

    // Confirm import
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        QStringLiteral("\u786e\u8ba4\u5bfc\u5165"), // 确认导入
        QStringLiteral("\u5bfc\u5165\u5c06\u8986\u76d6\u73b0\u6709\u6570\u636e\uff0c\u662f\u5426\u7ee7\u7eed\uff1f"), // 导入将覆盖现有数据，是否继续？
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    using namespace mcclock::services;

    // Clear existing data first (for overwrite mode)
    {
        AlarmService alarmSvc;
        for (const auto& a : alarmSvc.findAll()) {
            alarmSvc.hardDelete(a.uuid);
        }
    }
    {
        BirthdayService birthdaySvc;
        for (const auto& b : birthdaySvc.findAll()) {
            birthdaySvc.remove(b.uuid);
        }
    }
    {
        ShutdownService shutdownSvc;
        for (const auto& t : shutdownSvc.findAll()) {
            shutdownSvc.remove(t.uuid);
        }
    }
    {
        RunProgramService runSvc;
        for (const auto& t : runSvc.findAll()) {
            runSvc.remove(t.uuid);
        }
    }
    {
        CountdownService countdownSvc;
        for (const auto& c : countdownSvc.findAll()) {
            countdownSvc.remove(c.uuid);
        }
    }

    // Import alarms
    if (root.contains("alarms")) {
        QJsonArray arr = root["alarms"].toArray();
        AlarmService svc;
        for (const auto& v : arr) {
            auto a = mcclock::models::Alarm::fromJson(v.toObject());
            a.uuid.clear();
            svc.add(a);
        }
    }

    // Import birthdays
    if (root.contains("birthdays")) {
        QJsonArray arr = root["birthdays"].toArray();
        BirthdayService svc;
        for (const auto& v : arr) {
            auto b = mcclock::models::Birthday::fromJson(v.toObject());
            b.uuid.clear();
            svc.add(b);
        }
    }

    // Import shutdown tasks
    if (root.contains("shutdown_tasks")) {
        QJsonArray arr = root["shutdown_tasks"].toArray();
        ShutdownService svc;
        for (const auto& v : arr) {
            auto t = mcclock::models::ShutdownTask::fromJson(v.toObject());
            t.uuid.clear();
            svc.add(t);
        }
    }

    // Import run program tasks
    if (root.contains("run_programs")) {
        QJsonArray arr = root["run_programs"].toArray();
        RunProgramService svc;
        for (const auto& v : arr) {
            auto t = mcclock::models::RunProgramTask::fromJson(v.toObject());
            t.uuid.clear();
            svc.add(t);
        }
    }

    // Import countdowns
    if (root.contains("countdowns")) {
        QJsonArray arr = root["countdowns"].toArray();
        CountdownService svc;
        for (const auto& v : arr) {
            auto c = mcclock::models::Countdown::fromJson(v.toObject());
            c.uuid.clear();
            svc.add(c);
        }
    }

    // Import settings
    if (root.contains("settings")) {
        auto& s = mcclock::dal::SettingsManager::instance();
        s.load(mcclock::utils::PlatformUtils::settingsPath());
        QJsonObject settings = root["settings"].toObject();
        for (auto it = settings.begin(); it != settings.end(); ++it) {
            s.set({it.key()}, it.value());
        }
        s.save();
    }

    // Import sticky notes
    if (root.contains("sticky_notes")) {
        QString stickyPath = mcclock::utils::PlatformUtils::appDataPath() + "/sticky_notes.json";
        QFile sf(stickyPath);
        if (sf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            sf.write(QJsonDocument(root["sticky_notes"].toObject()).toJson());
        }
    }

    QMessageBox::information(this, QStringLiteral("\u6210\u529f"), // 成功
        QStringLiteral("\u6570\u636e\u5bfc\u5165\u5b8c\u6210")); // 数据导入完成

    // Reload settings to reflect imported data
    loadSettings();
    emit settingsSaved();
}

QWidget* SettingsDialog::createAboutTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(16);

    // App icon
    auto* iconLabel = new QLabel(page);
    iconLabel->setPixmap(ThemeManager::appIcon().pixmap(64, 64));
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    // App name
    auto* nameLabel = new QLabel(QStringLiteral("<b>MCClock梦畅闹钟</b>"), page);
    nameLabel->setStyleSheet("font-size: 18px; color: #2B2F33;");
    nameLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(nameLabel);

    // Description
    auto* descLabel = new QLabel(
        QStringLiteral("MCClock梦畅闹钟是一款采用C++ QT框架开发，支持CLI接口、API接口的多模块多功能、开源免费的闹钟软件。"),
        page);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #546E7A; font-size: 13px;");
    descLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(descLabel);

    // Repository link
    auto* repoLabel = new QLabel(
        QStringLiteral("官方仓库：<a href='https://github.com/hexiyou/MCClock'>https://github.com/hexiyou/MCClock</a>"),
        page);
    repoLabel->setOpenExternalLinks(true);
    repoLabel->setStyleSheet("color: #1E88E5; font-size: 13px;");
    repoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(repoLabel);

    // Divider
    auto* divider = new QFrame(page);
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Sunken);
    layout->addWidget(divider);

    // QR code section title
    auto* qrTitleLabel = new QLabel(QStringLiteral("【关注我们获取最新讯息】："), page);
    qrTitleLabel->setStyleSheet("color: #546E7A; font-size: 13px;");
    qrTitleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(qrTitleLabel);

    // Generate QR code placeholder
    QPixmap qrPixmap(120, 120);
    qrPixmap.fill(Qt::white);
    QPainter painter(&qrPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw QR code pattern (simplified)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#2B2F33"));

    // Random seed for unique pattern
    QRandomGenerator rng;
    int moduleSize = 6;
    int modules = 12;
    int margin = (120 - modules * moduleSize) / 2;

    // Draw position markers (corners)
    auto drawPositionMarker = [&](int x, int y) {
        // Outer black
        painter.setBrush(QColor("#2B2F33"));
        painter.drawRect(x, y, 7 * moduleSize, 7 * moduleSize);
        // Inner white
        painter.setBrush(Qt::white);
        painter.drawRect(x + moduleSize, y + moduleSize, 5 * moduleSize, 5 * moduleSize);
        // Center black
        painter.setBrush(QColor("#2B2F33"));
        painter.drawRect(x + 2 * moduleSize, y + 2 * moduleSize, 3 * moduleSize, 3 * moduleSize);
    };

    drawPositionMarker(margin, margin);
    drawPositionMarker(margin + 5 * moduleSize, margin);
    drawPositionMarker(margin, margin + 5 * moduleSize);

    // Draw random data modules
    painter.setBrush(QColor("#2B2F33"));
    for (int row = 0; row < modules; ++row) {
        for (int col = 0; col < modules; ++col) {
            // Skip position marker areas
            bool inMarker = (row < 8 && col < 8) ||
                           (row < 8 && col >= modules - 8) ||
                           (row >= modules - 8 && col < 8);
            if (inMarker) continue;

            if (rng.bounded(100) < 45) {
                int x = margin + col * moduleSize;
                int y = margin + row * moduleSize;
                painter.drawRect(x, y, moduleSize, moduleSize);
            }
        }
    }
    painter.end();

    auto* qrLabel = new QLabel(page);
    qrLabel->setPixmap(qrPixmap);
    qrLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(qrLabel);

    layout->addStretch();

    return page;
}

} // namespace mcclock::gui
