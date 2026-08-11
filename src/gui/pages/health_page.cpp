#include "health_page.h"
#include "widgets/reminder_popup.h"
#include "core/services/business_services.h"
#include "core/services/ringtone_manager.h"
#include "core/dal/settings_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QMouseEvent>
#include <QCloseEvent>

#include <functional>

namespace mcclock::gui {

using mcclock::services::HealthService;
using mcclock::services::RingtoneManager;

namespace {

// Fullscreen overlay reminder used when display mode is Fullscreen
class FullscreenReminder : public QWidget {
public:
    explicit FullscreenReminder(const QString& text, std::function<void()> onClose = nullptr)
        : onClose_(std::move(onClose)) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_DeleteOnClose);
        setStyleSheet("background-color: rgba(38, 50, 56, 235);");
        auto* layout = new QVBoxLayout(this);
        auto* label = new QLabel(text, this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #FFFFFF; font-size: 48px; font-weight: bold;");
        layout->addWidget(label);
        auto* hint = new QLabel(QStringLiteral("\u70b9\u51fb\u4efb\u610f\u5904\u5173\u95ed"), this); // 点击任意处关闭
        hint->setAlignment(Qt::AlignCenter);
        hint->setStyleSheet("color: #B0BEC5; font-size: 16px;");
        layout->addWidget(hint);
        auto* closeTimer = new QTimer(this);
        closeTimer->setSingleShot(true);
        connect(closeTimer, &QTimer::timeout, this, &FullscreenReminder::close);
        closeTimer->start(15000);
    }

protected:
    void mousePressEvent(QMouseEvent*) override { close(); }
    void closeEvent(QCloseEvent* e) override {
        if (onClose_) {
            onClose_();
            onClose_ = nullptr;
        }
        QWidget::closeEvent(e);
    }

private:
    std::function<void()> onClose_;
};

} // namespace

HealthPage::HealthPage(QWidget* parent)
    : QWidget(parent)
{
    ringtone_ = new RingtoneManager(this);
    setupUi();
    loadSettings();

    sessionTimer_ = new QTimer(this);
    sessionTimer_->setInterval(1000);
    connect(sessionTimer_, &QTimer::timeout, this, &HealthPage::onTick);
}

void HealthPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    enableCheck_ = new QCheckBox(QStringLiteral("\u542f\u7528\u5065\u5eb7\u63d0\u9192\uff08\u5de5\u4f5c/\u4f11\u606f\u5468\u671f\uff09"), this); // 启用健康提醒（工作/休息周期）
    form->addRow(enableCheck_);

    workSpin_ = new QSpinBox(this);
    workSpin_->setRange(1, 999);
    workSpin_->setValue(45);
    workSpin_->setSuffix(QStringLiteral(" \u5206\u949f")); // 分钟
    form->addRow(QStringLiteral("\u5de5\u4f5c\u65f6\u957f\uff1a"), workSpin_); // 工作时长：

    restSpin_ = new QSpinBox(this);
    restSpin_->setRange(1, 99);
    restSpin_->setValue(5);
    restSpin_->setSuffix(QStringLiteral(" \u5206\u949f"));
    form->addRow(QStringLiteral("\u4f11\u606f\u65f6\u957f\uff1a"), restSpin_); // 休息时长：

    displayCombo_ = new QComboBox(this);
    displayCombo_->addItem(QStringLiteral("\u7a97\u53e3\u63d0\u9192"), static_cast<int>(models::HealthDisplayMode::Window));     // 窗口提醒
    displayCombo_->addItem(QStringLiteral("\u5168\u5c4f\u63d0\u9192"), static_cast<int>(models::HealthDisplayMode::Fullscreen)); // 全屏提醒
    form->addRow(QStringLiteral("\u63d0\u9192\u65b9\u5f0f\uff1a"), displayCombo_); // 提醒方式：

    labelEdit_ = new QLineEdit(this);
    labelEdit_->setPlaceholderText(QStringLiteral("\u53ef\u9009\uff0c\u63d0\u9192\u9644\u52a0\u6587\u5b57")); // 可选，提醒附加文字
    form->addRow(QStringLiteral("\u5907\u6ce8\uff1a"), labelEdit_); // 备注：

    root->addLayout(form);

    auto* btnRow = new QHBoxLayout();
    auto* saveBtn = new QPushButton(QStringLiteral("\u4fdd\u5b58\u8bbe\u7f6e"), this); // 保存设置
    toggleBtn_ = new QPushButton(QStringLiteral("\u5f00\u59cb\u5de5\u4f5c\u5468\u671f"), this); // 开始工作周期
    toggleBtn_->setProperty("flatStyle", "secondary");
    auto* stopRingBtn = new QPushButton(QStringLiteral("\u505c\u6b62\u54cd\u94c3"), this); // 停止响铃
    stopRingBtn->setProperty("flatStyle", "danger");
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(toggleBtn_);
    btnRow->addWidget(stopRingBtn);
    btnRow->addStretch();
    root->addLayout(btnRow);

    statusLabel_ = new QLabel(QStringLiteral("\u672a\u5f00\u59cb"), this); // 未开始
    statusLabel_->setStyleSheet("font-size: 18px; color: #546E7A;");
    statusLabel_->setAlignment(Qt::AlignCenter);
    root->addWidget(statusLabel_, 1);

    connect(saveBtn, &QPushButton::clicked, this, &HealthPage::onSave);
    connect(toggleBtn_, &QPushButton::clicked, this, &HealthPage::onToggleSession);
    connect(stopRingBtn, &QPushButton::clicked, this, [this]() {
        ringtone_->stop();
    });
}

void HealthPage::loadSettings() {
    settings_ = HealthService().get();
    enableCheck_->setChecked(settings_.enabled);
    workSpin_->setValue(settings_.workMinutes);
    restSpin_->setValue(settings_.restMinutes);
    labelEdit_->setText(settings_.label);
    int idx = displayCombo_->findData(settings_.displayMode);
    if (idx >= 0) displayCombo_->setCurrentIndex(idx);
}

void HealthPage::onSave() {
    settings_.enabled = enableCheck_->isChecked();
    settings_.workMinutes = workSpin_->value();
    settings_.restMinutes = restSpin_->value();
    settings_.displayMode = displayCombo_->currentData().toInt();
    settings_.label = labelEdit_->text();
    HealthService().save(settings_);
    statusLabel_->setText(QStringLiteral("\u8bbe\u7f6e\u5df2\u4fdd\u5b58")); // 设置已保存
}

void HealthPage::onToggleSession() {
    if (sessionTimer_->isActive()) {
        sessionTimer_->stop();
        ringtone_->stop();
        toggleBtn_->setText(QStringLiteral("\u5f00\u59cb\u5de5\u4f5c\u5468\u671f")); // 开始工作周期
        statusLabel_->setText(QStringLiteral("\u5df2\u505c\u6b62")); // 已停止
        return;
    }
    settings_ = HealthService().get();
    if (!settings_.enabled) {
        enableCheck_->setChecked(true);
        settings_.enabled = true;
        settings_.workMinutes = workSpin_->value();
        settings_.restMinutes = restSpin_->value();
        HealthService().save(settings_);
    }
    inRestPhase_ = false;
    phaseSecondsLeft_ = settings_.workMinutes * 60;
    toggleBtn_->setText(QStringLiteral("\u505c\u6b62")); // 停止
    sessionTimer_->start();
}

void HealthPage::onTick() {
    phaseSecondsLeft_ -= 1;
    int m = phaseSecondsLeft_ / 60;
    int s = phaseSecondsLeft_ % 60;
    statusLabel_->setText(
        (inRestPhase_ ? QStringLiteral("\u4f11\u606f\u4e2d\uff1a") : QStringLiteral("\u5de5\u4f5c\u4e2d\uff1a")) // 休息中：/工作中：
        + QStringLiteral("%1:%2").arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0')));
    if (phaseSecondsLeft_ <= 0) {
        inRestPhase_ = !inRestPhase_;
        phaseSecondsLeft_ = (inRestPhase_ ? settings_.restMinutes : settings_.workMinutes) * 60;
        showPhaseReminder(inRestPhase_);
    }
}

void HealthPage::showPhaseReminder(bool restPhase) {
    auto& appSettings = mcclock::dal::SettingsManager::instance();
    ringtone_->play(settings_.ringtone, settings_.customRingtonePath, settings_.ringMode,
                    settings_.customMinutes, appSettings.alarmVolume());

    QString msg = restPhase
        ? QStringLiteral("\u5df2\u8fde\u7eed\u5de5\u4f5c %1 \u5206\u949f\uff0c\u8d77\u8eab\u4f11\u606f\u4e00\u4e0b\u5427")   // 已连续工作 X 分钟，起身休息一下吧
              .arg(settings_.workMinutes)
        : QStringLiteral("\u4f11\u606f\u7ed3\u675f\uff0c\u7ee7\u7eed\u52a0\u6cb9\uff01"); // 休息结束，继续加油！
    if (!settings_.label.isEmpty()) {
        msg += QStringLiteral("\uff08%1\uff09").arg(settings_.label); // （备注）
    }

    if (settings_.displayMode == static_cast<int>(models::HealthDisplayMode::Fullscreen)) {
        auto* overlay = new FullscreenReminder(msg, [this]() {
            ringtone_->stop(); // dismissing the overlay stops the ringtone
        });
        overlay->showFullScreen();
    } else {
        auto* popup = new ReminderPopup(QStringLiteral("\u5065\u5eb7\u63d0\u9192"), msg, nullptr); // 健康提醒
        connect(popup, &ReminderPopup::dismissClicked, this, [this]() {
            ringtone_->stop();
        });
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->showAtConfiguredPosition();
    }
}

} // namespace mcclock::gui
