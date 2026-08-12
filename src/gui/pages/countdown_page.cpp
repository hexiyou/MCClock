#include "countdown_page.h"
#include "widgets/reminder_popup.h"
#include "core/services/business_services.h"
#include "core/services/ringtone_manager.h"
#include "core/dal/settings_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QLabel>
#include <QDialog>
#include <QMessageBox>
#include <QTimer>

namespace mcclock::gui {

using mcclock::services::CountdownService;
using mcclock::services::RingtoneManager;

namespace {

QString formatHms(int totalSecs) {
    if (totalSecs < 0) totalSecs = 0;
    int h = totalSecs / 3600;
    int m = (totalSecs % 3600) / 60;
    int s = totalSecs % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

} // namespace

CountdownPage::CountdownPage(QWidget* parent)
    : QWidget(parent)
{
    ringtone_ = new RingtoneManager(this);
    setupUi();

    tickTimer_ = new QTimer(this);
    tickTimer_->setInterval(1000);
    connect(tickTimer_, &QTimer::timeout, this, &CountdownPage::onTick);
    tickTimer_->start();

    refresh();
}

void CountdownPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout();
    auto* addBtn = new QPushButton(QStringLiteral("\uff0b \u65b0\u589e\u5012\u8ba1\u65f6"), this); // ＋ 新增倒计时
    auto* editBtn = new QPushButton(QStringLiteral("\u7f16\u8f91"), this);       // 编辑
    auto* delBtn = new QPushButton(QStringLiteral("\u5220\u9664"), this);        // 删除
    auto* startBtn = new QPushButton(QStringLiteral("\u5f00\u59cb/\u505c\u6b62"), this); // 开始/停止
    auto* resetBtn = new QPushButton(QStringLiteral("\u91cd\u7f6e"), this);      // 重置
    editBtn->setProperty("flatStyle", "secondary");
    resetBtn->setProperty("flatStyle", "secondary");
    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(delBtn);
    toolbar->addWidget(startBtn);
    toolbar->addWidget(resetBtn);
    toolbar->addStretch();
    root->addLayout(toolbar);

    table_ = new QTableWidget(this);
    table_->setColumnCount(5);
    table_->setHorizontalHeaderLabels({
        QStringLiteral("\u5907\u6ce8"),       // 备注
        QStringLiteral("\u6a21\u5f0f"),       // 模式
        QStringLiteral("\u65f6\u957f/\u76ee\u6807"), // 时长/目标
        QStringLiteral("\u5269\u4f59"),       // 剩余
        QStringLiteral("\u72b6\u6001")        // 状态
    });
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);  // Allow Ctrl+Click multi-select
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->setAlternatingRowColors(true);
    root->addWidget(table_, 1);

    connect(addBtn, &QPushButton::clicked, this, &CountdownPage::addCountdown);
    connect(editBtn, &QPushButton::clicked, this, &CountdownPage::editSelected);
    connect(delBtn, &QPushButton::clicked, this, &CountdownPage::deleteSelected);
    connect(startBtn, &QPushButton::clicked, this, &CountdownPage::startStopSelected);
    connect(resetBtn, &QPushButton::clicked, this, &CountdownPage::resetSelected);
    connect(table_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { startStopSelected(); });
}

void CountdownPage::refresh() {
    auto items = CountdownService().findAll();
    table_->setRowCount(0);
    for (const auto& c : items) {
        int row = table_->rowCount();
        table_->insertRow(row);
        bool relative = c.mode == static_cast<int>(models::CountdownMode::Relative);
        table_->setItem(row, 0, new QTableWidgetItem(c.label));
        table_->setItem(row, 1, new QTableWidgetItem(
            relative ? QStringLiteral("\u76f8\u5bf9\u65f6\u957f")   // 相对时长
                     : QStringLiteral("\u76ee\u6807\u65f6\u95f4"))); // 目标时间
        table_->setItem(row, 2, new QTableWidgetItem(
            relative ? formatHms(c.totalSeconds) : QString(c.targetDatetime).replace('T', ' ').left(16)));
        table_->setItem(row, 3, new QTableWidgetItem(formatHms(c.remainingSeconds)));
        table_->setItem(row, 4, new QTableWidgetItem(
            c.enabled ? QStringLiteral("\u5c31\u7eea") : QStringLiteral("\u5df2\u505c\u7528"))); // 就绪 / 已停用
        table_->item(row, 0)->setData(Qt::UserRole, c.uuid);
    }
}

models::Countdown CountdownPage::currentSelected() {
    int row = table_->currentRow();
    if (row < 0) return {};
    QString uuid = table_->item(row, 0)->data(Qt::UserRole).toString();
    return CountdownService().findByUuid(uuid);
}

namespace {

// Shared edit form for countdown items
bool countdownForm(QWidget* parent, mcclock::models::Countdown& c, bool isNew) {
    QDialog dlg(parent);
    dlg.setWindowTitle(isNew ? QStringLiteral("\u65b0\u589e\u5012\u8ba1\u65f6")   // 新增倒计时
                             : QStringLiteral("\u7f16\u8f91\u5012\u8ba1\u65f6")); // 编辑倒计时
    dlg.resize(400, 260);
    auto* layout = new QFormLayout(&dlg);

    auto* labelEdit = new QLineEdit(c.label, &dlg);
    layout->addRow(QStringLiteral("\u5907\u6ce8\uff1a"), labelEdit); // 备注：

    auto* modeCombo = new QComboBox(&dlg);
    modeCombo->addItem(QStringLiteral("\u76f8\u5bf9\u65f6\u957f"), 0); // 相对时长
    modeCombo->addItem(QStringLiteral("\u76ee\u6807\u65f6\u95f4"), 1); // 目标时间

    auto* hourSpin = new QSpinBox(&dlg);
    hourSpin->setRange(0, 999);
    hourSpin->setSuffix(QStringLiteral(" \u65f6")); // 时
    auto* minSpin = new QSpinBox(&dlg);
    minSpin->setRange(0, 59);
    minSpin->setSuffix(QStringLiteral(" \u5206")); // 分
    auto* secSpin = new QSpinBox(&dlg);
    secSpin->setRange(0, 59);
    secSpin->setSuffix(QStringLiteral(" \u79d2")); // 秒
    if (!isNew && c.mode == static_cast<int>(mcclock::models::CountdownMode::Relative)) {
        hourSpin->setValue(c.totalSeconds / 3600);
        minSpin->setValue((c.totalSeconds % 3600) / 60);
        secSpin->setValue(c.totalSeconds % 60);
    } else {
        minSpin->setValue(5);
    }
    auto* durRow = new QHBoxLayout();
    durRow->addWidget(hourSpin);
    durRow->addWidget(minSpin);
    durRow->addWidget(secSpin);

    auto* targetEdit = new QDateTimeEdit(&dlg);
    targetEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    targetEdit->setCalendarPopup(true);
    if (!isNew && c.mode == static_cast<int>(mcclock::models::CountdownMode::Absolute)) {
        targetEdit->setDateTime(QDateTime::fromString(c.targetDatetime, Qt::ISODate));
    } else {
        targetEdit->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    }

    auto* relativeRow = new QWidget(&dlg);
    auto* relativeLayout = new QHBoxLayout(relativeRow);
    relativeLayout->setContentsMargins(0, 0, 0, 0);
    relativeLayout->addWidget(modeCombo);
    relativeLayout->addWidget(hourSpin);
    relativeLayout->addWidget(minSpin);
    relativeLayout->addWidget(secSpin);
    layout->addRow(QStringLiteral("\u65f6\u957f\uff1a"), relativeRow); // 时长：
    layout->addRow(QStringLiteral("\u76ee\u6807\uff1a"), targetEdit);  // 目标：

    auto applyMode = [modeCombo, relativeRow, targetEdit, hourSpin, minSpin, secSpin](int idx) {
        bool relative = idx == 0;
        hourSpin->setEnabled(relative);
        minSpin->setEnabled(relative);
        secSpin->setEnabled(relative);
        targetEdit->setEnabled(!relative);
    };
    QObject::connect(modeCombo, &QComboBox::currentIndexChanged, &dlg, applyMode);
    if (!isNew) modeCombo->setCurrentIndex(c.mode);
    applyMode(modeCombo->currentIndex());

    auto* btnRow = new QHBoxLayout();
    auto* okBtn = new QPushButton(QStringLiteral("\u4fdd\u5b58"), &dlg);   // 保存
    auto* cancelBtn = new QPushButton(QStringLiteral("\u53d6\u6d88"), &dlg); // 取消
    cancelBtn->setProperty("flatStyle", "secondary");
    btnRow->addStretch();
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    layout->addRow(btnRow);
    QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;

    c.label = labelEdit->text();
    c.mode = modeCombo->currentData().toInt();
    if (c.mode == static_cast<int>(mcclock::models::CountdownMode::Relative)) {
        c.totalSeconds = hourSpin->value() * 3600 + minSpin->value() * 60 + secSpin->value();
        if (c.totalSeconds <= 0) return false;
        c.targetDatetime.clear();
        c.remainingSeconds = c.totalSeconds;
    } else {
        if (targetEdit->dateTime() <= QDateTime::currentDateTime()) {
            QMessageBox::warning(&dlg, QStringLiteral("\u63d0\u793a"),
                QStringLiteral("\u76ee\u6807\u65f6\u95f4\u5fc5\u987b\u665a\u4e8e\u5f53\u524d\u65f6\u95f4")); // 目标时间必须晚于当前时间
            return false;
        }
        c.targetDatetime = targetEdit->dateTime().toString(Qt::ISODate);
        c.totalSeconds = 0;
        c.remainingSeconds = static_cast<int>(QDateTime::currentDateTime().secsTo(targetEdit->dateTime()));
    }
    return true;
}

} // namespace

void CountdownPage::addCountdown() {
    models::Countdown c;
    if (countdownForm(this, c, true)) {
        CountdownService().add(c);
        refresh();
        emit dataChanged();
    }
}

void CountdownPage::editSelected() {
    auto c = currentSelected();
    if (c.uuid.isEmpty()) return;
    if (countdownForm(this, c, false)) {
        CountdownService().update(c);
        refresh();
        emit dataChanged();
    }
}

void CountdownPage::deleteSelected() {
    QList<QTableWidgetItem*> selectedItems = table_->selectedItems();
    if (selectedItems.isEmpty()) return;

    // Get unique rows from selected items
    QSet<int> selectedRows;
    for (auto* item : selectedItems) {
        selectedRows.insert(item->row());
    }
    QList<int> rows = selectedRows.values();
    int count = rows.size();

    if (count == 0) return;

    if (count == 1) {
        auto c = currentSelected();
        if (c.uuid.isEmpty()) return;
        if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                QStringLiteral("\u786e\u5b9a\u5220\u9664\u8be5\u5012\u8ba1\u65f6\uff1f")) == QMessageBox::Yes) { // 确定删除该倒计时？
            CountdownService().remove(c.uuid);
        }
    } else {
        if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                QStringLiteral("\u786e\u5b9a\u5220\u9664 %1 \u4e2a\u5012\u8ba1\u65f6\uff1f").arg(count)) // 确定删除 X 个倒计时？
            == QMessageBox::Yes) {
            for (int row : rows) {
                QString uuid = table_->item(row, 0)->data(Qt::UserRole).toString();
                if (!uuid.isEmpty()) {
                    CountdownService().remove(uuid);
                }
            }
        }
    }
    refresh();
    emit dataChanged();
}

void CountdownPage::startStopSelected() {
    auto c = currentSelected();
    if (c.uuid.isEmpty()) return;
    CountdownService svc;

    bool relative = c.mode == static_cast<int>(models::CountdownMode::Relative);
    int remaining = relative ? c.remainingSeconds
                             : static_cast<int>(QDateTime::currentDateTime().secsTo(
                                   QDateTime::fromString(c.targetDatetime, Qt::ISODate)));
    if (c.enabled && remaining > 0) {
        // Stop: disable and persist current remaining
        c.enabled = false;
        c.remainingSeconds = remaining;
        svc.update(c);
    } else {
        // Start: if exhausted, restart from full duration / recompute target
        if (relative && c.remainingSeconds <= 0) c.remainingSeconds = c.totalSeconds;
        if (!relative) {
            c.remainingSeconds = static_cast<int>(QDateTime::currentDateTime().secsTo(
                QDateTime::fromString(c.targetDatetime, Qt::ISODate)));
        }
        c.enabled = true;
        svc.update(c);
        if (c.remainingSeconds <= 0) {
            finishCountdown(c);
            return;
        }
    }
    refresh();
    emit dataChanged();
}

void CountdownPage::resetSelected() {
    auto c = currentSelected();
    if (c.uuid.isEmpty()) return;
    if (c.mode != static_cast<int>(models::CountdownMode::Relative)) return;
    CountdownService svc;
    c.enabled = false;
    c.remainingSeconds = c.totalSeconds;
    svc.update(c);
    refresh();
    emit dataChanged();
}

void CountdownPage::onTick() {
    CountdownService svc;
    bool changed = false;
    for (auto& c : svc.findAll()) {
        if (!c.enabled) continue;
        int remaining = c.remainingSeconds;
        if (c.mode == static_cast<int>(models::CountdownMode::Absolute)) {
            remaining = static_cast<int>(QDateTime::currentDateTime().secsTo(
                QDateTime::fromString(c.targetDatetime, Qt::ISODate)));
        }
        remaining -= 1;
        if (remaining <= 0) {
            c.enabled = false;
            c.remainingSeconds = 0;
            svc.update(c);
            finishCountdown(c);
            changed = true;
        } else if (remaining != c.remainingSeconds) {
            svc.saveRemaining(c.uuid, remaining);
            changed = true;
        }
    }
    if (changed) {
        int row = table_->currentRow();
        refresh();
        if (row >= 0 && row < table_->rowCount()) table_->selectRow(row);
    }
}

void CountdownPage::finishCountdown(models::Countdown c) {
    auto& settings = mcclock::dal::SettingsManager::instance();
    ringtone_->play(c.ringtone, c.customRingtonePath, c.ringMode,
                    c.customMinutes, settings.alarmVolume());

    QString title = QStringLiteral("\u5012\u8ba1\u65f6\u7ed3\u675f"); // 倒计时结束
    QString msg = c.label.isEmpty()
        ? QStringLiteral("\u5012\u8ba1\u65f6\u5df2\u7ed3\u675f") // 倒计时已结束
        : QStringLiteral("\u5012\u8ba1\u65f6\u300c%1\u300d\u5df2\u7ed3\u675f").arg(c.label); // 倒计时「X」已结束
    auto* popup = new ReminderPopup(title, msg, nullptr);
    connect(popup, &ReminderPopup::dismissClicked, this, [this]() {
        ringtone_->stop();
    });
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->showAtConfiguredPosition();
}

} // namespace mcclock::gui
