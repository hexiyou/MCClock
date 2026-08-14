#include "alarm_page.h"
#include "dialogs/alarm_dialog.h"
#include "core/services/business_services.h"
#include "core/services/ringtone_manager.h"
#include "widgets/frameless_messagebox.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

namespace mcclock::gui {

using mcclock::services::AlarmService;
using mcclock::services::AlarmGroupService;
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
    auto* addBtn = new QPushButton(QStringLiteral("\uff0b \u65b0\u589e\u95f9\u949f"), this); // \uff0b \u65b0\u589e\u95f9\u949f
    editBtn_ = new QPushButton(QStringLiteral("\u7f16\u8f91"), this); // \u7f16\u8f91
    editBtn_->setProperty("flatStyle", "secondary");
    auto* delBtn = new QPushButton(QStringLiteral("\u5220\u9664"), this); // \u5220\u9664
    delBtn->setProperty("flatStyle", "danger");
    recycleBinBtn_ = new QPushButton(QStringLiteral("\u56de\u6536\u7ad9"), this); // \u56de\u6536\u7ad9
    recycleBinBtn_->setProperty("flatStyle", "secondary");
    
    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn_);
    toolbar->addWidget(delBtn);
    copyBtn_ = new QPushButton(QStringLiteral("\u590d\u5236"), this); // \u590d\u5236
    copyBtn_->setProperty("flatStyle", "secondary");
    toolbar->addWidget(copyBtn_);
    toolbar->addWidget(recycleBinBtn_);

    restoreBtn_ = new QPushButton(QStringLiteral("\u8fd8\u539f"), this); // 还原
    restoreBtn_->setProperty("flatStyle", "success");
    restoreBtn_->setVisible(false);
    toolbar->addWidget(restoreBtn_);

    clearRecycleBtn_ = new QPushButton(QStringLiteral("\u6e05\u7a7a\u56de\u6536\u7ad9"), this); // 清空回收站
    clearRecycleBtn_->setProperty("flatStyle", "danger");
    clearRecycleBtn_->setVisible(false);
    toolbar->addWidget(clearRecycleBtn_);

    toolbar->addStretch();

    sortCombo_ = new QComboBox(this);
    sortCombo_->addItem(QStringLiteral("\u6309\u65f6\u95f4\u6392\u5e8f"), "time");       // 按时间排序
    sortCombo_->addItem(QStringLiteral("\u6309\u521b\u5efa\u65f6\u95f4"), "created");     // 按创建时间
    sortCombo_->addItem(QStringLiteral("\u6309\u5907\u6ce8\u6392\u5e8f"), "label");       // 按备注排序
    toolbar->addWidget(sortCombo_);

    groupCombo_ = new QComboBox(this);
    refreshGroupCombo();
    toolbar->addWidget(groupCombo_);
    root->addLayout(toolbar);

    connect(addBtn, &QPushButton::clicked, this, &AlarmPage::addAlarm);
    connect(editBtn_, &QPushButton::clicked, this, &AlarmPage::editSelected);
    connect(delBtn, &QPushButton::clicked, this, &AlarmPage::deleteSelected);
    connect(copyBtn_, &QPushButton::clicked, this, &AlarmPage::copySelected);
    connect(recycleBinBtn_, &QPushButton::clicked, this, &AlarmPage::toggleRecycleBinView);
    connect(restoreBtn_, &QPushButton::clicked, this, &AlarmPage::restoreSelected);
    connect(clearRecycleBtn_, &QPushButton::clicked, this, &AlarmPage::clearRecycleBin);
    connect(sortCombo_, &QComboBox::currentIndexChanged, this, [this](int) { refresh(); });
    connect(groupCombo_, &QComboBox::currentIndexChanged, this, [this](int index) {
        QString data = groupCombo_->itemData(index).toString();
        if (data == "create") {
            createGroup();
        } else if (data == "manage") {
            manageGroups();
        } else {
            refresh();
        }
    });

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
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionsMovable(true);
    table_->horizontalHeader()->setMinimumSectionSize(50);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);  // Allow Ctrl+Click multi-select
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->setAlternatingRowColors(true);
    root->addWidget(table_, 1);

    connect(table_, &QTableWidget::cellDoubleClicked, this, &AlarmPage::onCellDoubleClicked);
    connect(table_->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, &AlarmPage::onHeaderDoubleClicked);
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
        FramelessMessageBox::information(this, QStringLiteral("\u63d0\u793a"),
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

void AlarmPage::copySelected() {
    if (recycleBinView_) {
        FramelessMessageBox::information(this, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u8bf7\u5148\u6062\u590d\u8be5\u95f9\u949f\u518d\u590d\u5236"));
        return;
    }
    // Check selection: exactly one row required
    QSet<int> selectedRows;
    for (auto* item : table_->selectedItems()) {
        selectedRows.insert(item->row());
    }
    if (selectedRows.isEmpty()) {
        FramelessMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u8bf7\u9009\u62e9\u4e00\u6761\u8981\u590d\u5236\u7684\u95f9\u949f"));
        return;
    }
    if (selectedRows.size() > 1) {
        FramelessMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u6bcf\u6b21\u53ea\u80fd\u590d\u5236\u4e00\u6761\u8bb0\u5f55"));
        return;
    }
    int row = *selectedRows.begin();
    QString uuid = table_->item(row, 1)->data(Qt::UserRole).toString();
    AlarmService svc;
    auto alarm = svc.findByUuid(uuid);
    if (alarm.uuid.isEmpty()) return;

    // Show input dialog for label
    bool ok = false;
    QString newLabel = FramelessInputDialog::getText(this, QStringLiteral("\u590d\u5236\u95f9\u949f"),
        QStringLiteral("\u8bf7\u8f93\u5165\u5907\u6ce8\u4fe1\u606f\uff1a"),
        QLineEdit::Normal, alarm.label, &ok);
    if (!ok) return;

    // Create a copy with new UUID and label
    alarm.uuid.clear();
    alarm.label = newLabel;
    alarm.createdAt.clear();
    alarm.lastModified.clear();
    svc.add(alarm);
    refresh();
    emit dataChanged();
}

void AlarmPage::deleteSelected() {
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

    AlarmService svc;

    if (recycleBinView_) {
        if (FramelessMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                QStringLiteral("\u786e\u5b9a\u6c38\u4e45\u5220\u9664 %1 \u4e2a\u95f9\u949f\uff1f").arg(count)) // 确定永久删除 X 个闹钟？
            == QMessageBox::Yes) {
            for (int row : rows) {
                QString uuid = table_->item(row, 1)->data(Qt::UserRole).toString();
                svc.hardDelete(uuid);
            }
        }
    } else {
        if (count == 1) {
            svc.moveToRecycleBin(table_->item(rows.first(), 1)->data(Qt::UserRole).toString());
        } else {
            if (FramelessMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                    QStringLiteral("\u786e\u5b9a\u5c06 %1 \u4e2a\u95f9\u949f\u79fb\u5230\u56de\u6536\u7ad9\uff1f").arg(count)) // 确定将 X 个闹钟移到回收站？
                == QMessageBox::Yes) {
                for (int row : rows) {
                    svc.moveToRecycleBin(table_->item(row, 1)->data(Qt::UserRole).toString());
                }
            }
        }
    }
    refresh();
    emit dataChanged();
}

void AlarmPage::toggleRecycleBinView() {
    recycleBinView_ = !recycleBinView_;
    recycleBinBtn_->setText(recycleBinView_
        ? QStringLiteral("\u8fd4\u56de\u5217\u8868")       // 返回列表
        : QStringLiteral("\u56de\u6536\u7ad9"));            // 回收站
    editBtn_->setVisible(!recycleBinView_);
    copyBtn_->setVisible(!recycleBinView_);
    restoreBtn_->setVisible(recycleBinView_);
    clearRecycleBtn_->setVisible(recycleBinView_);
    refresh();
}

void AlarmPage::restoreSelected() {
    QList<QTableWidgetItem*> selectedItems = table_->selectedItems();
    if (selectedItems.isEmpty()) return;

    QSet<int> selectedRows;
    for (auto* item : selectedItems) {
        selectedRows.insert(item->row());
    }
    QList<int> rows = selectedRows.values();
    int count = rows.size();

    if (count == 0) return;

    AlarmService svc;
    if (count == 1) {
        svc.restore(table_->item(rows.first(), 1)->data(Qt::UserRole).toString());
    } else {
        if (FramelessMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                QStringLiteral("\u786e\u5b9a\u8fd8\u539f %1 \u4e2a\u95f9\u949f\uff1f").arg(count))
            == QMessageBox::Yes) {
            for (int row : rows) {
                svc.restore(table_->item(row, 1)->data(Qt::UserRole).toString());
            }
        }
    }
    refresh();
    emit dataChanged();
}

void AlarmPage::clearRecycleBin() {
    if (FramelessMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
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

void AlarmPage::refreshGroupCombo() {
    groupCombo_->blockSignals(true);
    groupCombo_->clear();
    groupCombo_->addItem(QStringLiteral("\u5168\u90e8\u5206\u7ec4"), "all"); // 全部分组

    // Load groups from database
    auto groups = AlarmGroupService().findAll();
    for (const auto& g : groups) {
        groupCombo_->addItem(g.name, g.uuid);
    }

    // Add separator and management options
    groupCombo_->insertSeparator(groupCombo_->count());
    groupCombo_->addItem(QStringLiteral("\u2b50 \u521b\u5efa\u5206\u7ec4"), "create"); // 创建分组
    groupCombo_->addItem(QStringLiteral("\u2699 \u7ba1\u7406\u5206\u7ec4"), "manage"); // 管理分组
    groupCombo_->blockSignals(false);
}

void AlarmPage::createGroup() {
    bool ok;
    QString name = FramelessInputDialog::getText(this, QStringLiteral("\u521b\u5efa\u5206\u7ec4"), // 创建分组
        QStringLiteral("\u8bf7\u8f93\u5165\u5206\u7ec4\u540d\u79f0\uff1a"), // 请输入分组名称：
        QLineEdit::Normal, QString(), &ok);

    if (ok && !name.trimmed().isEmpty()) {
        mcclock::models::AlarmGroup group;
        group.name = name.trimmed();
        AlarmGroupService().add(group);

        // Refresh group combo and select the new group
        refreshGroupCombo();
        for (int i = 0; i < groupCombo_->count(); ++i) {
            if (groupCombo_->itemText(i) == name.trimmed()) {
                groupCombo_->setCurrentIndex(i);
                break;
            }
        }
    } else {
        // Reset to previous selection
        groupCombo_->setCurrentIndex(0);
    }
}

void AlarmPage::manageGroups() {
    auto groups = AlarmGroupService().findAll();
    if (groups.isEmpty()) {
        FramelessMessageBox::information(this, QStringLiteral("\u7ba1\u7406\u5206\u7ec4"), // 管理分组
            QStringLiteral("\u6ca1\u6709\u53ef\u7ba1\u7406\u7684\u5206\u7ec4")); // 没有可管理的分组
        groupCombo_->setCurrentIndex(0);
        return;
    }

    // Simple dialog to manage groups
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("\u7ba1\u7406\u5206\u7ec4")); // 管理分组
    dialog.setMinimumWidth(300);

    auto* layout = new QVBoxLayout(&dialog);
    auto* listWidget = new QListWidget(&dialog);

    for (const auto& g : groups) {
        auto* item = new QListWidgetItem(g.name, listWidget);
        item->setData(Qt::UserRole, g.uuid);
    }
    layout->addWidget(listWidget);

    auto* btnLayout = new QHBoxLayout();
    auto* renameBtn = new QPushButton(QStringLiteral("\u91cd\u547d\u540d"), &dialog); // 重命名
    auto* deleteBtn = new QPushButton(QStringLiteral("\u5220\u9664"), &dialog);       // 删除
    deleteBtn->setProperty("flatStyle", "danger");
    btnLayout->addWidget(renameBtn);
    btnLayout->addWidget(deleteBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(renameBtn, &QPushButton::clicked, [&]() {
        auto* item = listWidget->currentItem();
        if (!item) return;
        QString uuid = item->data(Qt::UserRole).toString();
        bool ok;
        QString newName = FramelessInputDialog::getText(&dialog, QStringLiteral("\u91cd\u547d\u540d\u5206\u7ec4"), // 重命名分组
            QStringLiteral("\u65b0\u540d\u79f0\uff1a"), // 新名称：
            QLineEdit::Normal, item->text(), &ok);
        if (ok && !newName.trimmed().isEmpty()) {
            AlarmGroupService().rename(uuid, newName.trimmed());
            item->setText(newName.trimmed());
        }
    });

    connect(deleteBtn, &QPushButton::clicked, [&]() {
        auto* item = listWidget->currentItem();
        if (!item) return;
        QString uuid = item->data(Qt::UserRole).toString();
        if (FramelessMessageBox::question(&dialog, QStringLiteral("\u786e\u8ba4\u5220\u9664"), // 确认删除
                QStringLiteral("\u5220\u9664\u5206\u7ec4\u540e\uff0c\u8be5\u5206\u7ec4\u4e0b\u7684\u95f9\u949f\u5c06\u79fb\u5230\u9ed8\u8ba4\u5206\u7ec4\uff0c\u662f\u5426\u7ee7\u7eed\uff1f")) // 删除分组后，该分组下的闹钟将移到默认分组，是否继续？
            == QMessageBox::Yes) {
            AlarmGroupService().remove(uuid);
            delete item;
        }
    });

    dialog.exec();

    // Refresh and reset selection
    refreshGroupCombo();
    groupCombo_->setCurrentIndex(0);
}

void AlarmPage::onHeaderDoubleClicked(int logicalIndex) {
    if (sortColumn_ == logicalIndex) {
        sortOrder_ = (sortOrder_ == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        sortColumn_ = logicalIndex;
        sortOrder_ = Qt::AscendingOrder;
    }
    table_->setSortingEnabled(true);
    table_->sortItems(sortColumn_, sortOrder_);
    table_->setSortingEnabled(false);
}

} // namespace mcclock::gui
