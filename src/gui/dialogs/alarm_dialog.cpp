#include "alarm_dialog.h"
#include "core/services/ringtone_manager.h"
#include "core/services/business_services.h"
#include "core/utils/time_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QTimeEdit>
#include <QDateEdit>
#include <QLineEdit>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace mcclock::gui {

using mcclock::services::RingtoneManager;

AlarmDialog::AlarmDialog(const mcclock::models::Alarm& existing, QWidget* parent)
    : QDialog(parent), alarm_(existing)
{
    editing_ = !alarm_.uuid.isEmpty();
    setWindowTitle(editing_ ? QStringLiteral("\u7f16\u8f91\u95f9\u949f")     // 编辑闹钟
                            : QStringLiteral("\u65b0\u589e\u95f9\u949f"));   // 新增闹钟
    resize(460, 480);
    setupUi();
    loadFromModel();
}

void AlarmDialog::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    enabledCheck_ = new QCheckBox(QStringLiteral("\u542f\u7528\u95f9\u949f"), this); // 启用闹钟
    enabledCheck_->setChecked(true);
    form->addRow(enabledCheck_);

    // Time
    timeEdit_ = new QTimeEdit(this);
    timeEdit_->setDisplayFormat("HH:mm");
    form->addRow(QStringLiteral("\u95f9\u949f\u65f6\u95f4\uff1a"), timeEdit_); // 闹钟时间：

    // Cycle mode
    cycleCombo_ = new QComboBox(this);
    cycleCombo_->addItem(QStringLiteral("\u53ea\u54cd\u4e00\u6b21"), 0);      // 只响一次
    cycleCombo_->addItem(QStringLiteral("\u6bcf\u5929"), 1);                  // 每天
    cycleCombo_->addItem(QStringLiteral("\u6bcf\u5468"), 2);                  // 每周
    cycleCombo_->addItem(QStringLiteral("\u6bcf\u6708"), 3);                  // 每月
    cycleCombo_->addItem(QStringLiteral("\u6bcf\u5e74"), 4);                  // 每年
    cycleCombo_->addItem(QStringLiteral("\u95f4\u9694\u91cd\u590d"), 5);      // 间隔重复
    form->addRow(QStringLiteral("\u91cd\u590d\u5468\u671f\uff1a"), cycleCombo_); // 重复周期：

    // Cycle options stacked widget
    cycleStack_ = new QStackedWidget(this);

    // Page 0: once - date
    auto* oncePage = new QWidget(this);
    auto* onceLayout = new QHBoxLayout(oncePage);
    onceLayout->setContentsMargins(0, 0, 0, 0);
    onceDateEdit_ = new QDateEdit(oncePage);
    onceDateEdit_->setDisplayFormat("yyyy-MM-dd");
    onceDateEdit_->setCalendarPopup(true);
    onceLayout->addWidget(onceDateEdit_);
    onceLayout->addStretch();
    cycleStack_->addWidget(oncePage);

    // Page 1: daily - no options
    auto* dailyPage = new QWidget(this);
    auto* dailyLayout = new QHBoxLayout(dailyPage);
    dailyLayout->setContentsMargins(0, 0, 0, 0);
    dailyLayout->addWidget(new QLabel(QStringLiteral("\u6bcf\u5929\u56fa\u5b9a\u65f6\u95f4\u89e6\u53d1"), dailyPage)); // 每天固定时间触发
    dailyLayout->addStretch();
    cycleStack_->addWidget(dailyPage);

    // Page 2: weekly - 7 checkboxes
    auto* weeklyPage = new QWidget(this);
    auto* weeklyLayout = new QHBoxLayout(weeklyPage);
    weeklyLayout->setContentsMargins(0, 0, 0, 0);
    const QStringList dayNames = {
        QStringLiteral("\u4e00"), QStringLiteral("\u4e8c"), QStringLiteral("\u4e09"),
        QStringLiteral("\u56db"), QStringLiteral("\u4e94"), QStringLiteral("\u516d"),
        QStringLiteral("\u65e5")
    };
    for (const auto& n : dayNames) {
        auto* check = new QCheckBox(QStringLiteral("\u5468") + n, weeklyPage); // 周X
        weeklyLayout->addWidget(check);
        weekdayChecks_.append(check);
    }
    weeklyLayout->addStretch();
    cycleStack_->addWidget(weeklyPage);

    // Page 3: monthly - day spin
    auto* monthlyPage = new QWidget(this);
    auto* monthlyLayout = new QHBoxLayout(monthlyPage);
    monthlyLayout->setContentsMargins(0, 0, 0, 0);
    monthlyLayout->addWidget(new QLabel(QStringLiteral("\u6bcf\u6708"), monthlyPage)); // 每月
    monthDaySpin_ = new QSpinBox(monthlyPage);
    monthDaySpin_->setRange(1, 31);
    monthlyLayout->addWidget(monthDaySpin_);
    monthlyLayout->addWidget(new QLabel(QStringLiteral("\u65e5"), monthlyPage)); // 日
    monthlyLayout->addStretch();
    cycleStack_->addWidget(monthlyPage);

    // Page 4: yearly - month + day
    auto* yearlyPage = new QWidget(this);
    auto* yearlyLayout = new QHBoxLayout(yearlyPage);
    yearlyLayout->setContentsMargins(0, 0, 0, 0);
    yearlyLayout->addWidget(new QLabel(QStringLiteral("\u6bcf\u5e74"), yearlyPage)); // 每年
    yearMonthSpin_ = new QSpinBox(yearlyPage);
    yearMonthSpin_->setRange(1, 12);
    yearlyLayout->addWidget(yearMonthSpin_);
    yearlyLayout->addWidget(new QLabel(QStringLiteral("\u6708"), yearlyPage)); // 月
    yearDaySpin_ = new QSpinBox(yearlyPage);
    yearDaySpin_->setRange(1, 31);
    yearlyLayout->addWidget(yearDaySpin_);
    yearlyLayout->addWidget(new QLabel(QStringLiteral("\u65e5"), yearlyPage)); // 日
    yearlyLayout->addStretch();
    cycleStack_->addWidget(yearlyPage);

    // Page 5: interval - minutes spin
    auto* intervalPage = new QWidget(this);
    auto* intervalLayout = new QHBoxLayout(intervalPage);
    intervalLayout->setContentsMargins(0, 0, 0, 0);
    intervalLayout->addWidget(new QLabel(QStringLiteral("\u6bcf\u95f4\u9694"), intervalPage)); // 每间隔
    intervalSpin_ = new QSpinBox(intervalPage);
    intervalSpin_->setRange(1, 100000);
    intervalSpin_->setValue(30);
    intervalLayout->addWidget(intervalSpin_);
    intervalLayout->addWidget(new QLabel(QStringLiteral("\u5206\u949f\u91cd\u590d\u4e00\u6b21"), intervalPage)); // 分钟重复一次
    intervalLayout->addStretch();
    cycleStack_->addWidget(intervalPage);

    form->addRow(QStringLiteral(" "), cycleStack_);

    // Date range
    rangeCheck_ = new QCheckBox(QStringLiteral("\u9650\u5b9a\u65e5\u671f\u8303\u56f4"), this); // 限定日期范围
    form->addRow(rangeCheck_);
    auto* rangeRow = new QHBoxLayout();
    rangeStartEdit_ = new QDateEdit(this);
    rangeStartEdit_->setDisplayFormat("yyyy-MM-dd");
    rangeStartEdit_->setCalendarPopup(true);
    rangeEndEdit_ = new QDateEdit(this);
    rangeEndEdit_->setDisplayFormat("yyyy-MM-dd");
    rangeEndEdit_->setCalendarPopup(true);
    rangeRow->addWidget(rangeStartEdit_);
    rangeRow->addWidget(new QLabel(QStringLiteral("\u81f3"), this)); // 至
    rangeRow->addWidget(rangeEndEdit_);
    rangeRow->addStretch();
    form->addRow(QStringLiteral(" "), rangeRow);

    // Ringtone
    auto* ringRow = new QHBoxLayout();
    ringtoneCombo_ = new QComboBox(this);
    for (int i = 1; i <= RingtoneManager::builtinCount(); ++i) {
        ringtoneCombo_->addItem(RingtoneManager::builtinName(i), i);
    }
    ringRow->addWidget(ringtoneCombo_, 1);
    previewBtn_ = new QPushButton(QStringLiteral("\u8bd5\u542c"), this); // 试听
    connect(previewBtn_, &QPushButton::clicked, this, &AlarmDialog::previewRingtone);
    ringRow->addWidget(previewBtn_);
    form->addRow(QStringLiteral("\u94c3\u58f0\uff1a"), ringRow); // 铃声：

    customPathEdit_ = new QLineEdit(this);
    customPathEdit_->setPlaceholderText(QStringLiteral("\u81ea\u5b9a\u4e49\u97f3\u9891\u6587\u4ef6\u8def\u5f84")); // 自定义音频文件路径
    auto* browseRow = new QHBoxLayout();
    browseRow->addWidget(customPathEdit_, 1);
    auto* browseBtn = new QPushButton(QStringLiteral("\u6d4f\u89c8..."), this); // 浏览...
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString f = QFileDialog::getOpenFileName(this,
            QStringLiteral("\u9009\u62e9\u97f3\u9891\u6587\u4ef6"), QString(), // 选择音频文件
            QStringLiteral("Audio (*.mp3 *.wav *.wma *.m4a *.ogg)"));
        if (!f.isEmpty()) customPathEdit_->setText(f);
    });
    browseRow->addWidget(browseBtn);
    form->addRow(QStringLiteral(" "), browseRow);

    // Ring mode
    auto* ringModeRow = new QHBoxLayout();
    ringModeCombo_ = new QComboBox(this);
    ringModeCombo_->addItem(QStringLiteral("\u62a5\u65f6\u540e\u54cd\u94c3"), 0);   // 报时后响铃
    ringModeCombo_->addItem(QStringLiteral("\u6301\u7eed\u54cd\u94c3"), 1);          // 持续响铃
    ringModeCombo_->addItem(QStringLiteral("\u54cd\u94c3\u4e00\u6b21"), 2);          // 响铃一次
    ringModeCombo_->addItem(QStringLiteral("\u9759\u97f3"), 3);                      // 静音
    ringModeCombo_->addItem(QStringLiteral("\u81ea\u5b9a\u4e49\u65f6\u957f"), 4);    // 自定义时长
    ringModeRow->addWidget(ringModeCombo_);
    customMinutesSpin_ = new QSpinBox(this);
    customMinutesSpin_->setRange(1, 120);
    customMinutesSpin_->setSuffix(QStringLiteral(" \u5206\u949f")); // 分钟
    ringModeRow->addWidget(customMinutesSpin_);
    ringModeRow->addStretch();
    form->addRow(QStringLiteral("\u54cd\u94c3\u65b9\u5f0f\uff1a"), ringModeRow); // 响铃方式：

    // Label
    labelEdit_ = new QLineEdit(this);
    labelEdit_->setPlaceholderText(QStringLiteral("\u95f9\u949f\u5907\u6ce8\uff08\u53ef\u9009\uff09")); // 闹钟备注（可选）
    form->addRow(QStringLiteral("\u5907\u6ce8\uff1a"), labelEdit_); // 备注：

    // Group selection
    groupCombo_ = new QComboBox(this);
    form->addRow(QStringLiteral("\u5206\u7ec4\uff1a"), groupCombo_); // 分组：

    root->addLayout(form);
    root->addStretch();

    // Buttons
    auto* btnBar = new QHBoxLayout();
    btnBar->addStretch();
    auto* saveBtn = new QPushButton(QStringLiteral("\u4fdd\u5b58"), this); // 保存
    auto* cancelBtn = new QPushButton(QStringLiteral("\u53d6\u6d88"), this); // 取消
    cancelBtn->setProperty("flatStyle", "secondary");
    btnBar->addWidget(saveBtn);
    btnBar->addWidget(cancelBtn);
    root->addLayout(btnBar);

    connect(saveBtn, &QPushButton::clicked, this, &AlarmDialog::save);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(cycleCombo_, &QComboBox::currentIndexChanged, this, &AlarmDialog::onCycleModeChanged);
    connect(rangeCheck_, &QCheckBox::toggled, rangeStartEdit_, &QWidget::setEnabled);
    connect(rangeCheck_, &QCheckBox::toggled, rangeEndEdit_, &QWidget::setEnabled);
    rangeStartEdit_->setEnabled(false);
    rangeEndEdit_->setEnabled(false);

    // Show/hide custom minutes spinbox based on ring mode
    // Mode 0 (announce then ring) and 4 (custom duration) need the spinbox
    auto updateMinutesVisibility = [this](int index) {
        int mode = ringModeCombo_->itemData(index).toInt();
        customMinutesSpin_->setVisible(mode == 0 || mode == 4);
    };
    connect(ringModeCombo_, &QComboBox::currentIndexChanged, this, updateMinutesVisibility);
    updateMinutesVisibility(ringModeCombo_->currentIndex());
}

void AlarmDialog::onCycleModeChanged(int index) {
    cycleStack_->setCurrentIndex(index);
}

void AlarmDialog::previewRingtone() {
    int id = ringtoneCombo_->currentData().toInt();
    static RingtoneManager previewer;
    if (previewer.isPlaying()) {
        previewer.stop();
        previewBtn_->setText(QStringLiteral("\u8bd5\u542c")); // 试听
    } else {
        previewer.play(id, customPathEdit_->text(),
                       static_cast<int>(mcclock::models::RingMode::Once), 0, 80);
        previewBtn_->setText(QStringLiteral("\u505c\u6b62")); // 停止
    }
}

void AlarmDialog::loadFromModel() {
    enabledCheck_->setChecked(alarm_.enabled);
    timeEdit_->setTime(QTime::fromString(alarm_.time, "HH:mm").isValid()
        ? QTime::fromString(alarm_.time, "HH:mm") : QTime(8, 0));
    cycleCombo_->setCurrentIndex(alarm_.cycleMode);
    cycleStack_->setCurrentIndex(alarm_.cycleMode);

    QJsonObject cd = QJsonDocument::fromJson(alarm_.cycleData.toUtf8()).object();
    switch (alarm_.cycleMode) {
    case 0: {
        QDate d = QDate::fromString(cd.value("date").toString(), "yyyy-MM-dd");
        onceDateEdit_->setDate(d.isValid() ? d : QDate::currentDate());
        break;
    }
    case 2: {
        QJsonArray days = cd.value("weekdays").toArray();
        for (int i = 0; i < weekdayChecks_.size(); ++i) {
            weekdayChecks_[i]->setChecked(days.contains(QJsonValue(i + 1)));
        }
        break;
    }
    case 3:
        monthDaySpin_->setValue(cd.value("day").toInt(1));
        break;
    case 4:
        yearMonthSpin_->setValue(cd.value("month").toInt(1));
        yearDaySpin_->setValue(cd.value("day").toInt(1));
        break;
    case 5:
        intervalSpin_->setValue(cd.value("interval_minutes").toInt(30));
        break;
    default:
        break;
    }

    bool hasRange = !alarm_.rangeStart.isEmpty() || !alarm_.rangeEnd.isEmpty();
    rangeCheck_->setChecked(hasRange);
    if (hasRange) {
        rangeStartEdit_->setDate(QDate::fromString(alarm_.rangeStart.left(10), "yyyy-MM-dd"));
        rangeEndEdit_->setDate(QDate::fromString(alarm_.rangeEnd.left(10), "yyyy-MM-dd"));
    } else {
        rangeStartEdit_->setDate(QDate::currentDate());
        rangeEndEdit_->setDate(QDate::currentDate().addMonths(1));
    }

    ringtoneCombo_->setCurrentIndex(ringtoneCombo_->findData(alarm_.ringtone));
    customPathEdit_->setText(alarm_.customRingtonePath);
    ringModeCombo_->setCurrentIndex(ringModeCombo_->findData(alarm_.ringMode));
    customMinutesSpin_->setValue(alarm_.customMinutes > 0 ? alarm_.customMinutes : 1);
    labelEdit_->setText(alarm_.label);

    // Load groups into combo box
    groupCombo_->clear();
    groupCombo_->addItem(QStringLiteral("\u9ed8\u8ba4\u5206\u7ec4"), "default"); // 默认分组
    auto groups = mcclock::services::AlarmGroupService().findAll();
    for (const auto& g : groups) {
        groupCombo_->addItem(g.name, g.uuid);
    }

    // Select current group
    QString groupId = alarm_.groupId.isEmpty() ? "default" : alarm_.groupId;
    int groupIndex = groupCombo_->findData(groupId);
    if (groupIndex >= 0) {
        groupCombo_->setCurrentIndex(groupIndex);
    }
}

QString AlarmDialog::buildCycleData() const {
    QJsonObject cd;
    switch (cycleCombo_->currentIndex()) {
    case 0:
        cd["date"] = onceDateEdit_->date().toString("yyyy-MM-dd");
        break;
    case 2: {
        QJsonArray days;
        for (int i = 0; i < weekdayChecks_.size(); ++i) {
            if (weekdayChecks_[i]->isChecked()) days.append(i + 1);
        }
        cd["weekdays"] = days;
        break;
    }
    case 3:
        cd["day"] = monthDaySpin_->value();
        break;
    case 4:
        cd["month"] = yearMonthSpin_->value();
        cd["day"] = yearDaySpin_->value();
        break;
    case 5:
        cd["interval_minutes"] = intervalSpin_->value();
        // Keep existing anchor, or create new one at today's time
        if (alarm_.cycleData.contains("anchor")) {
            QJsonObject old = QJsonDocument::fromJson(alarm_.cycleData.toUtf8()).object();
            cd["anchor"] = old.value("anchor");
        } else {
            QDateTime anchor(QDate::currentDate(), timeEdit_->time());
            cd["anchor"] = anchor.toString(Qt::ISODateWithMs);
        }
        break;
    default:
        break;
    }
    return QString::fromUtf8(QJsonDocument(cd).toJson(QJsonDocument::Compact));
}

void AlarmDialog::save() {
    // Validation
    if (cycleCombo_->currentIndex() == 2) {
        bool any = false;
        for (auto* c : weekdayChecks_) if (c->isChecked()) { any = true; break; }
        if (!any) {
            QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                QStringLiteral("\u8bf7\u81f3\u5c11\u9009\u62e9\u4e00\u4e2a\u661f\u671f")); // 请至少选择一个星期
            return;
        }
    }
    if (ringtoneCombo_->currentData().toInt() == 8 && customPathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u8bf7\u9009\u62e9\u81ea\u5b9a\u4e49\u94c3\u58f0\u6587\u4ef6")); // 请选择自定义铃声文件
        return;
    }

    alarm_.enabled = enabledCheck_->isChecked();
    alarm_.time = timeEdit_->time().toString("HH:mm");
    alarm_.cycleMode = cycleCombo_->currentIndex();
    alarm_.cycleData = buildCycleData();
    if (rangeCheck_->isChecked()) {
        alarm_.rangeStart = rangeStartEdit_->date().toString("yyyy-MM-dd") + "T00:00:00";
        alarm_.rangeEnd = rangeEndEdit_->date().toString("yyyy-MM-dd") + "T23:59:59";
    } else {
        alarm_.rangeStart.clear();
        alarm_.rangeEnd.clear();
    }
    alarm_.ringtone = ringtoneCombo_->currentData().toInt();
    alarm_.customRingtonePath = customPathEdit_->text();
    alarm_.ringMode = ringModeCombo_->currentData().toInt();
    alarm_.customMinutes = customMinutesSpin_->value();
    alarm_.label = labelEdit_->text();
    alarm_.groupId = groupCombo_->currentData().toString();
    if (alarm_.groupId.isEmpty()) alarm_.groupId = "default";

    accept();
}

} // namespace mcclock::gui
