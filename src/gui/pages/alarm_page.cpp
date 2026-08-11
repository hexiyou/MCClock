#include "alarm_page.h"
#include "dialogs/alarm_dialog.h"
#include "core/services/business_services.h"
#include "core/services/ringtone_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

namespace mcclock::gui {

using mcclock::services::AlarmService;
using mcclock::services::RingtoneManager;

AlarmPage::AlarmPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refresh();
}

void AlarmPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    auto* addBtn = new QPushButton(QStringLiteral("\uff0b \u65b0\u589e\u95f9\u949f"), this); // ＋ 新增闹钟
    auto* editBtn = new QPushButton(QStringLiteral("\u7f16\u8f91"), this); // 编辑
    editBtn->setProperty("flatStyle", "secondary");
    auto* delBtn = new QPushButton(QStringLiteral("\u5220\u9664"), this); // 删除
    delBtn->setProperty("flatStyle", "danger");
    recycleBinBtn_ = new QPushButton(QStringLiteral("\u56de\u6536\u7ad9"), this); // 回收站
    recycleBinBtn_->setProperty("flatStyle", "secondary");

    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(delBtn);
    toolbar->addWidget(recycleBinBtn_);
    toolbar->addStretch();

    sortCombo_ = new QComboBox(this);
    sortCombo_->addItem(QStringLiteral("\u6309\u65f6\u95f4\u6392\u5e8f"), "time");       // 按时间排序
    sortCombo_->addItem(QStringLiteral("\u6309\u521b\u5efa\u65f6\u95f4"), "created");     // 按创建时间
    sortCombo_->addItem(QStringLiteral("\u6309\u5907\u6ce8\u6392\u5e8f"), "label");       // 按备注排序
    toolbar->addWidget(sortCombo_);

    groupCombo_ = new QComboBox(this);
    groupCombo_->addItem(QStringLiteral("\u5168\u90e8\u5206\u7ec4"), "all"); // 全部分组
    groupCombo_->addItem(QStringLiteral("\u9ed8\u8ba4"), "default");         // 默认
    toolbar->addWidget(groupCombo_);
    root->addLayout(toolbar);

    connect(addBtn, &QPushButton::clicked, this, &AlarmPage::addAlarm);
    connect(editBtn, &QPushButton::clicked, this, &AlarmPage::editSelected);
    connect(delBtn, &QPushButton::clicked, this, &AlarmPage::deleteSelected);
    connect(recycleBinBtn_, &QPushButton::clicked, this, &AlarmPage::toggleRecycleBinView);
    connect(sortCombo_, &QComboBox::currentIndexChanged, this, [this](int) { refresh(); });
    connect(groupCombo_, &QComboBox::currentIndexChanged, this, [this](int) { refresh(); });

    // Table
    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({
        QStringLiteral("\u542f\u7528"),   // 启用
        QStringLiteral("\u65f6\u95f4"),   // 时间
        QStringLiteral("\u91cd\u590d"),   // 重复
        QStringLiteral("\u94c3\u58f0"),   // 铃声
        QStringLiteral("\u5907\u6ce8"),   // 备注
        QStringLiteral("\u6700\u540e\u4fee\u6539") // 最后修改
    });
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->setAlternatingRowColors(true);
    root->addWidget(table_, 1);

    connect(table_, &QTableWidget::cellDoubleClicked, this, &AlarmPage::onCellDoubleClicked);
    connect(table_, &QTableWidget::cellClicked, this, [this](int row, int col) {
        if (col == 0) onEnableToggled(row);
    });
}

QString AlarmPage::cycleDescription(const mcclock::models::Alarm& a) const {
    QJsonObject cd = QJsonDocument::fromJson(a.cycleData.toUtf8()).object();
    switch (a.cycleMode) {
    case 0: return QStringLiteral("\u53ea\u54cd\u4e00\u6b21 %1").arg(cd.value("date").toString()); // 只响一次
    case 1: return QStringLiteral("\u6bcf\u5929"); // 每天
    case 2: {
        static const char* names[] = { "\xe4\xb8\x80", "\xe4\xba\x8c", "\xe4\xb8\x89",
            "\xe5\x9b\x9b", "\xe4\xba\x94", "\xe5\x85\xad", "\xe6\x97\xa5" };
        QString s = QStringLiteral("\u5468"); // 周
        const QJsonArray days = cd.value("weekdays").toArray();
        QStringList parts;
        for (const auto& v : days) {
            int d = v.toInt();
            if (d >= 1 && d <= 7) parts << QString::fromUtf8(names[d - 1]);
        }
        return s + parts.join(",");
    }
    case 3: return QStringLiteral("\u6bcf\u6708%1\u65e5").arg(cd.value("day").toInt()); // 每月X日
    case 4: return QStringLiteral("\u6bcf\u5e74%1\u6708%2\u65e5") // 每年X月X日
        .arg(cd.value("month").toInt()).arg(cd.value("day").toInt());
    case 5: return QStringLiteral("\u6bcf%1\u5206\u949f").arg(cd.value("interval_minutes").toInt()); // 每X分钟
    default: return QString();
    }
}

void AlarmPage::refresh() {
    AlarmService svc;
    QList<mcclock::models::Alarm> alarms;
    if (recycleBinView_) {
        alarms = svc.findDeleted();
    } else {
        QString group = groupCombo_->currentData().toString();
        alarms = (group == "all") ? svc.findAll() : svc.findByGroup(group);
    }

    // Sorting
    QString sortBy = sortCombo_->currentData().toString();
    std::sort(alarms.begin(), alarms.end(),
        [sortBy](const mcclock::models::Alarm& x, const mcclock::models::Alarm& y) {
        if (sortBy == "time") return x.time < y.time;
        if (sortBy == "label") return x.label < y.label;
        return x.createdAt > y.createdAt;
    });

    table_->setRowCount(0);
    for (const auto& a : alarms) {
        int row = table_->rowCount();
        table_->insertRow(row);

        auto* enableItem = new QTableWidgetItem(a.enabled
            ? QStringLiteral("\u2714") : QStringLiteral("\u2716")); // ✔ / ✖
        enableItem->setTextAlignment(Qt::AlignCenter);
        enableItem->setForeground(a.enabled ? QColor("#1E88E5") : QColor("#B0BEC5"));
        table_->setItem(row, 0, enableItem);

        auto* timeItem = new QTableWidgetItem(a.time);
        timeItem->setTextAlignment(Qt::AlignCenter);
        QFont f = timeItem->font();
        f.setPointSize(12);
        f.setBold(true);
        timeItem->setFont(f);
        table_->setItem(row, 1, timeItem);

        table_->setItem(row, 2, new QTableWidgetItem(cycleDescription(a)));
        table_->setItem(row, 3, new QTableWidgetItem(RingtoneManager::builtinName(a.ringtone)));
        table_->setItem(row, 4, new QTableWidgetItem(a.label));
        table_->setItem(row, 5, new QTableWidgetItem(a.lastModified.left(16)));

        table_->item(row, 1)->setData(Qt::UserRole, a.uuid);
    }
}

void AlarmPage::addAlarm() {
    AlarmDialog dlg({}, this);
    if (dlg.exec() == QDialog::Accepted) {
        AlarmService svc;
        svc.add(dlg.alarm());
        refresh();
        emit dataChanged();
    }
}

void AlarmPage::editSelected() {
    if (recycleBinView_) {
        QMessageBox::information(this, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u8bf7\u5148\u6062\u590d\u8be5\u95f9\u949f\u518d\u7f16\u8f91")); // 请先恢复该闹钟再编辑
        return;
    }
    int row = table_->currentRow();
    if (row < 0) return;
    QString uuid = table_->item(row, 1)->data(Qt::UserRole).toString();
    AlarmService svc;
    auto alarm = svc.findByUuid(uuid);
    if (alarm.uuid.isEmpty()) return;

    AlarmDialog dlg(alarm, this);
    if (dlg.exec() == QDialog::Accepted) {
        svc.update(dlg.alarm());
        refresh();
        emit dataChanged();
    }
}

void AlarmPage::deleteSelected() {
    int row = table_->currentRow();
    if (row < 0) return;
    QString uuid = table_->item(row, 1)->data(Qt::UserRole).toString();
    AlarmService svc;

    if (recycleBinView_) {
        if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                QStringLiteral("\u786e\u5b9a\u6c38\u4e45\u5220\u9664\u8be5\u95f9\u949f\uff1f")) // 确定永久删除该闹钟？
            == QMessageBox::Yes) {
            svc.hardDelete(uuid);
        }
    } else {
        svc.moveToRecycleBin(uuid);
    }
    refresh();
    emit dataChanged();
}

void AlarmPage::toggleRecycleBinView() {
    recycleBinView_ = !recycleBinView_;
    recycleBinBtn_->setText(recycleBinView_
        ? QStringLiteral("\u8fd4\u56de\u5217\u8868")       // 返回列表
        : QStringLiteral("\u56de\u6536\u7ad9"));            // 回收站
    refresh();
}

void AlarmPage::clearRecycleBin() {
    if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
            QStringLiteral("\u786e\u5b9a\u6e05\u7a7a\u56de\u6536\u7ad9\uff1f")) == QMessageBox::Yes) { // 确定清空回收站？
        AlarmService().clearRecycleBin();
        refresh();
        emit dataChanged();
    }
}

void AlarmPage::onCellDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    if (recycleBinView_) return;
    table_->setCurrentCell(row, 0);
    editSelected();
}

void AlarmPage::onEnableToggled(int row) {
    if (recycleBinView_) return;
    QString uuid = table_->item(row, 1)->data(Qt::UserRole).toString();
    AlarmService svc;
    auto alarm = svc.findByUuid(uuid);
    if (alarm.uuid.isEmpty()) return;
    svc.setEnabled(uuid, !alarm.enabled);
    refresh();
    emit dataChanged();
}

} // namespace mcclock::gui
