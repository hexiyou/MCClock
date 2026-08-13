#include "task_pages.h"
#include "core/services/business_services.h"
#include "core/services/cycle_utils.h"
#include "core/utils/platform_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QComboBox>
#include <QTimeEdit>
#include <QDateEdit>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QColor>

namespace mcclock::gui {

using mcclock::services::ShutdownService;
using mcclock::services::RunProgramService;

// ── Shared helper: cycle editor row (simplified: once/daily/weekly) ──
namespace {

struct CycleEditor {
    QComboBox* combo = nullptr;
    QDateEdit* onceDateEdit = nullptr;   // date for "run once" mode
    QLineEdit* weeklineEdit = nullptr; // e.g. "1,3,5" (1=Mon..7=Sun)
    QSpinBox* intervalHourSpin = nullptr;
    QSpinBox* intervalMinSpin = nullptr;
    QSpinBox* intervalSecSpin = nullptr;

    QWidget* create(QWidget* parent) {
        auto* w = new QWidget(parent);
        auto* layout = new QHBoxLayout(w);
        layout->setContentsMargins(0, 0, 0, 0);
        combo = new QComboBox(w);
        combo->addItem(QStringLiteral("\u53ea\u6267\u884c\u4e00\u6b21"), 0); // 只执行一次
        combo->addItem(QStringLiteral("\u6bcf\u5929"), 1);                   // 每天
        combo->addItem(QStringLiteral("\u6bcf\u5468"), 2);                   // 每周
        combo->addItem(QStringLiteral("\u6bcf\u9694..."), 3);                // 每隔...
        layout->addWidget(combo);
        onceDateEdit = new QDateEdit(w);
        onceDateEdit->setDisplayFormat("yyyy-MM-dd");
        onceDateEdit->setDate(QDate::currentDate());
        layout->addWidget(onceDateEdit);
        weeklineEdit = new QLineEdit(w);
        weeklineEdit->setPlaceholderText(QStringLiteral("\u661f\u671f\uff0c\u5982 1,3,5\uff081=\u5468\u4e00\uff09")); // 星期，如 1,3,5（1=周一）
        weeklineEdit->setVisible(false);
        layout->addWidget(weeklineEdit, 1);

        // Interval spin boxes
        auto* intervalWidget = new QWidget(w);
        auto* intervalLayout = new QHBoxLayout(intervalWidget);
        intervalLayout->setContentsMargins(0, 0, 0, 0);
        intervalHourSpin = new QSpinBox(intervalWidget);
        intervalHourSpin->setRange(0, 99);
        intervalHourSpin->setSuffix(QStringLiteral(" \u65f6")); // 时
        intervalMinSpin = new QSpinBox(intervalWidget);
        intervalMinSpin->setRange(0, 59);
        intervalMinSpin->setSuffix(QStringLiteral(" \u5206")); // 分
        intervalSecSpin = new QSpinBox(intervalWidget);
        intervalSecSpin->setRange(0, 59);
        intervalSecSpin->setSuffix(QStringLiteral(" \u79d2")); // 秒
        intervalLayout->addWidget(intervalHourSpin);
        intervalLayout->addWidget(intervalMinSpin);
        intervalLayout->addWidget(intervalSecSpin);
        intervalWidget->setVisible(false);
        layout->addWidget(intervalWidget);

        QObject::connect(combo, &QComboBox::currentIndexChanged, w, [this, intervalWidget](int idx) {
            onceDateEdit->setVisible(idx == 0);
            weeklineEdit->setVisible(idx == 2);
            intervalWidget->setVisible(idx == 3);
        });
        return w;
    }

    void load(int cycleMode, const QString& cycleData) {
        QJsonObject cd = QJsonDocument::fromJson(cycleData.toUtf8()).object();
        if (cycleMode == 3) {
            combo->setCurrentIndex(3);
            onceDateEdit->setVisible(false);
            weeklineEdit->setVisible(false);
            int hours = cd.value("interval_hours").toInt();
            int minutes = cd.value("interval_minutes").toInt();
            int seconds = cd.value("interval_seconds").toInt();
            intervalHourSpin->setValue(hours);
            intervalMinSpin->setValue(minutes);
            intervalSecSpin->setValue(seconds);
        } else {
            combo->setCurrentIndex(cycleMode >= 0 && cycleMode <= 2 ? cycleMode : 0);
            QDate d = QDate::fromString(cd.value("date").toString(), "yyyy-MM-dd");
            onceDateEdit->setDate(d.isValid() ? d : QDate::currentDate());
            QStringList parts;
            for (const auto& v : cd.value("weekdays").toArray()) parts << QString::number(v.toInt());
            weeklineEdit->setText(parts.join(","));
        }
    }

    QString buildCycleData(int& mode) const {
        mode = combo->currentData().toInt();
        QJsonObject cd;
        if (mode == 0) {
            cd["date"] = onceDateEdit->date().toString("yyyy-MM-dd");
        } else if (mode == 2) {
            QJsonArray arr;
            for (const auto& part : weeklineEdit->text().split(',', Qt::SkipEmptyParts)) {
                bool ok = false;
                int d = part.trimmed().toInt(&ok);
                if (ok && d >= 1 && d <= 7) arr.append(d);
            }
            cd["weekdays"] = arr;
        } else if (mode == 3) {
            cd["interval_hours"] = intervalHourSpin->value();
            cd["interval_minutes"] = intervalMinSpin->value();
            cd["interval_seconds"] = intervalSecSpin->value();
        }
        return QString::fromUtf8(QJsonDocument(cd).toJson(QJsonDocument::Compact));
    }

    static QString describe(int mode, const QString& cycleData) {
        if (mode == 1) return QStringLiteral("\u6bcf\u5929"); // 每天
        if (mode == 2) {
            QJsonObject cd = QJsonDocument::fromJson(cycleData.toUtf8()).object();
            QStringList parts;
            for (const auto& v : cd.value("weekdays").toArray()) parts << QString::number(v.toInt());
            return QStringLiteral("\u5468 ") + parts.join(","); // 周 X,X
        }
        if (mode == 3) {
            QJsonObject cd = QJsonDocument::fromJson(cycleData.toUtf8()).object();
            int h = cd.value("interval_hours").toInt();
            int m = cd.value("interval_minutes").toInt();
            int s = cd.value("interval_seconds").toInt();
            QStringList parts;
            if (h > 0) parts << QStringLiteral("%1\u65f6").arg(h);
            if (m > 0) parts << QStringLiteral("%1\u5206").arg(m);
            if (s > 0) parts << QStringLiteral("%1\u79d2").arg(s);
            return QStringLiteral("\u6bcf\u9694 ") + parts.join(""); // 每隔 X时X分X秒
        }
        return QStringLiteral("\u4e00\u6b21"); // 一次
    }
};

} // namespace

// ── ShutdownPage ──

ShutdownPage::ShutdownPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refresh();
}

void ShutdownPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout();
    auto* addBtn = new QPushButton(QStringLiteral("\uff0b \u65b0\u589e\u5173\u673a\u4efb\u52a1"), this); // ＋ 新增关机任务
    auto* editBtn = new QPushButton(QStringLiteral("\u7f16\u8f91"), this);     // 编辑
    auto* delBtn = new QPushButton(QStringLiteral("\u5220\u9664"), this);      // 删除
    auto* runBtn = new QPushButton(QStringLiteral("\u7acb\u5373\u6267\u884c"), this); // 立即执行
    editBtn->setProperty("flatStyle", "secondary");
    runBtn->setProperty("flatStyle", "danger");
    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(delBtn);
    toolbar->addWidget(runBtn);
    toolbar->addStretch();
    root->addLayout(toolbar);

    table_ = new QTableWidget(this);
    table_->setColumnCount(5);
    table_->setHorizontalHeaderLabels({
        QStringLiteral("\u65f6\u95f4"),       // 时间
        QStringLiteral("\u91cd\u590d"),       // 重复
        QStringLiteral("\u64cd\u4f5c"),       // 操作
        QStringLiteral("\u63d0\u524d\u8b66\u544a"), // 提前警告
        QStringLiteral("\u5907\u6ce8")        // 备注
    });
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);  // Allow Ctrl+Click multi-select
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->setAlternatingRowColors(true);
    root->addWidget(table_, 1);

    connect(addBtn, &QPushButton::clicked, this, &ShutdownPage::addTask);
    connect(editBtn, &QPushButton::clicked, this, &ShutdownPage::editSelected);
    connect(delBtn, &QPushButton::clicked, this, &ShutdownPage::deleteSelected);
    connect(runBtn, &QPushButton::clicked, this, &ShutdownPage::executeSelected);
    connect(table_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { editSelected(); });
}

void ShutdownPage::refresh() {
    auto tasks = ShutdownService().findAll();
    table_->setRowCount(0);
    static const char* optionNames[] = {
        "\xe5\xbc\xba\xe5\x88\xb6\xe5\x85\xb3\xe6\x9c\xba",   // 强制关机
        "\xe6\xad\xa3\xe5\xb8\xb8\xe5\x85\xb3\xe6\x9c\xba",   // 正常关机
        "\xe9\x87\x8d\xe5\x90\xaf",                           // 重启
        "\xe6\xb3\xa8\xe9\x94\x80"                            // 注销
    };
    for (const auto& t : tasks) {
        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new QTableWidgetItem(t.time));
        table_->setItem(row, 1, new QTableWidgetItem(CycleEditor::describe(t.cycleMode, t.cycleData)));
        int opt = t.shutdownOption;
        QString optName = (opt >= 0 && opt <= 3) ? QString::fromUtf8(optionNames[opt]) : QString();
        table_->setItem(row, 2, new QTableWidgetItem(optName));
        table_->setItem(row, 3, new QTableWidgetItem(QStringLiteral("%1 \u79d2").arg(t.advanceSeconds))); // X 秒
        table_->setItem(row, 4, new QTableWidgetItem(t.label));
        table_->item(row, 0)->setData(Qt::UserRole, t.uuid);
    }
}

void ShutdownPage::addTask() {
    mcclock::models::ShutdownTask t;
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("\u65b0\u589e\u5173\u673a\u4efb\u52a1")); // 新增关机任务
    dlg.resize(380, 260);
    auto* layout = new QFormLayout(&dlg);

    auto* timeEdit = new QTimeEdit(&dlg);
    timeEdit->setDisplayFormat("HH:mm");
    layout->addRow(QStringLiteral("\u6267\u884c\u65f6\u95f4\uff1a"), timeEdit); // 执行时间：

    CycleEditor cycle;
    layout->addRow(QStringLiteral("\u91cd\u590d\uff1a"), cycle.create(&dlg)); // 重复：

    auto* optionCombo = new QComboBox(&dlg);
    optionCombo->addItem(QStringLiteral("\u5f3a\u5236\u5173\u673a"), 0);  // 强制关机
    optionCombo->addItem(QStringLiteral("\u6b63\u5e38\u5173\u673a"), 1);  // 正常关机
    optionCombo->addItem(QStringLiteral("\u91cd\u542f"), 2);              // 重启
    optionCombo->addItem(QStringLiteral("\u6ce8\u9500"), 3);              // 注销
    optionCombo->setCurrentIndex(1);
    layout->addRow(QStringLiteral("\u5173\u673a\u9009\u9879\uff1a"), optionCombo); // 关机选项：

    auto* advanceSpin = new QSpinBox(&dlg);
    advanceSpin->setRange(0, 600);
    advanceSpin->setValue(30);
    advanceSpin->setSuffix(QStringLiteral(" \u79d2")); // 秒
    layout->addRow(QStringLiteral("\u63d0\u524d\u8b66\u544a\uff1a"), advanceSpin); // 提前警告：

    auto* labelEdit = new QLineEdit(&dlg);
    layout->addRow(QStringLiteral("\u5907\u6ce8\uff1a"), labelEdit); // 备注：

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

    if (dlg.exec() == QDialog::Accepted) {
        t.time = timeEdit->time().toString("HH:mm");
        t.cycleData = cycle.buildCycleData(t.cycleMode);
        t.shutdownOption = optionCombo->currentData().toInt();
        t.advanceSeconds = advanceSpin->value();
        t.label = labelEdit->text();
        ShutdownService().add(t);
        refresh();
        emit dataChanged();
    }
}

void ShutdownPage::editSelected() {
    int row = table_->currentRow();
    if (row < 0) return;
    QString uuid = table_->item(row, 0)->data(Qt::UserRole).toString();
    ShutdownService svc;
    auto t = svc.findByUuid(uuid);
    if (t.uuid.isEmpty()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("\u7f16\u8f91\u5173\u673a\u4efb\u52a1")); // 编辑关机任务
    dlg.resize(380, 260);
    auto* layout = new QFormLayout(&dlg);

    auto* timeEdit = new QTimeEdit(&dlg);
    timeEdit->setDisplayFormat("HH:mm");
    timeEdit->setTime(QTime::fromString(t.time, "HH:mm"));
    layout->addRow(QStringLiteral("\u6267\u884c\u65f6\u95f4\uff1a"), timeEdit);

    CycleEditor cycle;
    layout->addRow(QStringLiteral("\u91cd\u590d\uff1a"), cycle.create(&dlg));
    cycle.load(t.cycleMode, t.cycleData);

    auto* optionCombo = new QComboBox(&dlg);
    optionCombo->addItem(QStringLiteral("\u5f3a\u5236\u5173\u673a"), 0);
    optionCombo->addItem(QStringLiteral("\u6b63\u5e38\u5173\u673a"), 1);
    optionCombo->addItem(QStringLiteral("\u91cd\u542f"), 2);
    optionCombo->addItem(QStringLiteral("\u6ce8\u9500"), 3);
    optionCombo->setCurrentIndex(t.shutdownOption);
    layout->addRow(QStringLiteral("\u5173\u673a\u9009\u9879\uff1a"), optionCombo);

    auto* advanceSpin = new QSpinBox(&dlg);
    advanceSpin->setRange(0, 600);
    advanceSpin->setValue(t.advanceSeconds);
    advanceSpin->setSuffix(QStringLiteral(" \u79d2"));
    layout->addRow(QStringLiteral("\u63d0\u524d\u8b66\u544a\uff1a"), advanceSpin);

    auto* labelEdit = new QLineEdit(t.label, &dlg);
    layout->addRow(QStringLiteral("\u5907\u6ce8\uff1a"), labelEdit);

    auto* btnRow = new QHBoxLayout();
    auto* okBtn = new QPushButton(QStringLiteral("\u4fdd\u5b58"), &dlg);
    auto* cancelBtn = new QPushButton(QStringLiteral("\u53d6\u6d88"), &dlg);
    cancelBtn->setProperty("flatStyle", "secondary");
    btnRow->addStretch();
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    layout->addRow(btnRow);
    QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        t.time = timeEdit->time().toString("HH:mm");
        t.cycleData = cycle.buildCycleData(t.cycleMode);
        t.shutdownOption = optionCombo->currentData().toInt();
        t.advanceSeconds = advanceSpin->value();
        t.label = labelEdit->text();
        svc.update(t);
        refresh();
        emit dataChanged();
    }
}

void ShutdownPage::deleteSelected() {
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
        QString uuid = table_->item(rows.first(), 0)->data(Qt::UserRole).toString();
        if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                QStringLiteral("\u786e\u5b9a\u5220\u9664\u8be5\u4efb\u52a1\uff1f")) == QMessageBox::Yes) { // 确定删除该任务？
            ShutdownService().remove(uuid);
        }
    } else {
        if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                QStringLiteral("\u786e\u5b9a\u5220\u9664 %1 \u4e2a\u4efb\u52a1\uff1f").arg(count)) // 确定删除 X 个任务？
            == QMessageBox::Yes) {
            for (int row : rows) {
                ShutdownService().remove(table_->item(row, 0)->data(Qt::UserRole).toString());
            }
        }
    }
    refresh();
    emit dataChanged();
}

void ShutdownPage::executeSelected() {
    int row = table_->currentRow();
    if (row < 0) return;
    QString uuid = table_->item(row, 0)->data(Qt::UserRole).toString();
    ShutdownService svc;
    auto t = svc.findByUuid(uuid);
    if (t.uuid.isEmpty()) return;
    if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
            QStringLiteral("\u7acb\u5373\u6267\u884c\u8be5\u5173\u673a\u64cd\u4f5c\uff1f")) == QMessageBox::Yes) { // 立即执行该关机操作？
        svc.executeNow(t);
    }
}

// ── RunProgramPage ──

RunProgramPage::RunProgramPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refresh();
}

void RunProgramPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout();
    auto* addBtn = new QPushButton(QStringLiteral("\uff0b \u65b0\u589e\u4efb\u52a1"), this); // ＋ 新增任务
    auto* editBtn = new QPushButton(QStringLiteral("\u7f16\u8f91"), this);     // 编辑
    auto* delBtn = new QPushButton(QStringLiteral("\u5220\u9664"), this);      // 删除
    auto* testBtn = new QPushButton(QStringLiteral("\u8bd5\u8fd0\u884c"), this); // 试运行
    editBtn->setProperty("flatStyle", "secondary");
    testBtn->setProperty("flatStyle", "secondary");
    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(delBtn);
    toolbar->addWidget(testBtn);
    toolbar->addStretch();
    root->addLayout(toolbar);

    table_ = new QTableWidget(this);
    table_->setColumnCount(5);
    table_->setHorizontalHeaderLabels({
        QStringLiteral("\u65f6\u95f4"),       // 时间
        QStringLiteral("\u91cd\u590d"),       // 重复
        QStringLiteral("\u7a0b\u5e8f/\u7f51\u5740"), // 程序/网址
        QStringLiteral("\u72b6\u6001"),       // 状态
        QStringLiteral("\u5907\u6ce8")        // 备注
    });
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);  // Allow Ctrl+Click multi-select
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->setAlternatingRowColors(true);
    root->addWidget(table_, 1);

    connect(addBtn, &QPushButton::clicked, this, &RunProgramPage::addTask);
    connect(editBtn, &QPushButton::clicked, this, &RunProgramPage::editSelected);
    connect(delBtn, &QPushButton::clicked, this, &RunProgramPage::deleteSelected);
    connect(testBtn, &QPushButton::clicked, this, &RunProgramPage::testRunSelected);
    connect(table_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { editSelected(); });
}

void RunProgramPage::refresh() {
    auto tasks = RunProgramService().findAll();
    table_->setRowCount(0);
    for (const auto& t : tasks) {
        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new QTableWidgetItem(t.time));
        table_->setItem(row, 1, new QTableWidgetItem(CycleEditor::describe(t.cycleMode, t.cycleData)));
        table_->setItem(row, 2, new QTableWidgetItem(t.programPath));
        auto* statusItem = new QTableWidgetItem(t.enabled
            ? QStringLiteral("\u542f\u7528")   // 启用
            : QStringLiteral("\u7981\u7528")); // 禁用
        if (!t.enabled) statusItem->setForeground(QColor("#90A4AE"));
        table_->setItem(row, 3, statusItem);
        table_->setItem(row, 4, new QTableWidgetItem(t.label));
        table_->item(row, 0)->setData(Qt::UserRole, t.uuid);
    }
}

namespace {

// Shared edit form for run program tasks
bool runProgramForm(QWidget* parent, mcclock::models::RunProgramTask& t, bool isNew) {
    QDialog dlg(parent);
    dlg.setWindowTitle(isNew ? QStringLiteral("\u65b0\u589e\u8fd0\u884c\u7a0b\u5e8f\u4efb\u52a1")   // 新增运行程序任务
                              : QStringLiteral("\u7f16\u8f91\u8fd0\u884c\u7a0b\u5e8f\u4efb\u52a1")); // 编辑运行程序任务
    dlg.resize(440, 280);
    auto* layout = new QFormLayout(&dlg);

    auto* timeEdit = new QTimeEdit(&dlg);
    timeEdit->setDisplayFormat("HH:mm");
    if (!isNew) timeEdit->setTime(QTime::fromString(t.time, "HH:mm"));
    layout->addRow(QStringLiteral("\u6267\u884c\u65f6\u95f4\uff1a"), timeEdit); // 执行时间：

    CycleEditor cycle;
    layout->addRow(QStringLiteral("\u91cd\u590d\uff1a"), cycle.create(&dlg)); // 重复：
    if (!isNew) cycle.load(t.cycleMode, t.cycleData);

    auto* pathRow = new QHBoxLayout();
    auto* pathEdit = new QLineEdit(t.programPath, &dlg);
    pathEdit->setPlaceholderText(QStringLiteral("\u7a0b\u5e8f\u8def\u5f84\u6216\u7f51\u5740\uff08http://...\uff09")); // 程序路径或网址
    auto* browseBtn = new QPushButton(QStringLiteral("\u6d4f\u89c8..."), &dlg); // 浏览...
    pathRow->addWidget(pathEdit, 1);
    pathRow->addWidget(browseBtn);
    layout->addRow(QStringLiteral("\u7a0b\u5e8f\uff1a"), pathRow); // 程序：
    QObject::connect(browseBtn, &QPushButton::clicked, &dlg, [pathEdit, &dlg]() {
        QString f = QFileDialog::getOpenFileName(&dlg,
            QStringLiteral("\u9009\u62e9\u7a0b\u5e8f"), QString(), // 选择程序
            QStringLiteral("Executable (*.exe *.bat *.cmd);;All Files (*)"));
        if (!f.isEmpty()) pathEdit->setText(f);
    });

    auto* argsEdit = new QLineEdit(t.arguments, &dlg);
    argsEdit->setPlaceholderText(QStringLiteral("\u652f\u6301\u73af\u5883\u53d8\u91cf\uff0c\u5982 %COMSPEC%")); // 支持环境变量，如 %COMSPEC%
    layout->addRow(QStringLiteral("\u53c2\u6570\uff1a"), argsEdit); // 参数：

    auto* labelEdit = new QLineEdit(t.label, &dlg);
    layout->addRow(QStringLiteral("\u5907\u6ce8\uff1a"), labelEdit); // 备注：

    auto* enabledCheck = new QCheckBox(QStringLiteral("\u542f\u7528\u8be5\u4efb\u52a1"), &dlg); // 启用该任务
    enabledCheck->setChecked(isNew ? true : t.enabled);
    layout->addRow(QStringLiteral("\u72b6\u6001\uff1a"), enabledCheck); // 状态：

    auto* btnRow = new QHBoxLayout();
    auto* okBtn = new QPushButton(QStringLiteral("\u4fdd\u5b58"), &dlg);
    auto* cancelBtn = new QPushButton(QStringLiteral("\u53d6\u6d88"), &dlg);
    cancelBtn->setProperty("flatStyle", "secondary");
    btnRow->addStretch();
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    layout->addRow(btnRow);
    QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;

    QString path = pathEdit->text().trimmed();
    if (path.isEmpty()) return false;
    // URL detection: warn but allow; expand environment variables before existence check
    const QString expandedPath = mcclock::utils::PlatformUtils::expandEnvVars(path);
    if (!RunProgramService::isUrl(path) && !QFile::exists(expandedPath)) {
        if (QMessageBox::question(&dlg, QStringLiteral("\u63d0\u793a"),
                QStringLiteral("\u6587\u4ef6\u4e0d\u5b58\u5728\uff0c\u4ecd\u8981\u4fdd\u5b58\uff1f")) != QMessageBox::Yes) { // 文件不存在，仍要保存？
            return false;
        }
    }
    t.time = timeEdit->time().toString("HH:mm");
    t.cycleData = cycle.buildCycleData(t.cycleMode);
    t.programPath = path;
    t.arguments = argsEdit->text();
    t.label = labelEdit->text();
    t.enabled = enabledCheck->isChecked();
    return true;
}

} // namespace

void RunProgramPage::addTask() {
    mcclock::models::RunProgramTask t;
    if (runProgramForm(this, t, true)) {
        RunProgramService().add(t);
        refresh();
        emit dataChanged();
    }
}

void RunProgramPage::editSelected() {
    int row = table_->currentRow();
    if (row < 0) return;
    QString uuid = table_->item(row, 0)->data(Qt::UserRole).toString();
    RunProgramService svc;
    auto t = svc.findByUuid(uuid);
    if (t.uuid.isEmpty()) return;
    if (runProgramForm(this, t, false)) {
        svc.update(t);
        refresh();
        emit dataChanged();
    }
}

void RunProgramPage::deleteSelected() {
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
        QString uuid = table_->item(rows.first(), 0)->data(Qt::UserRole).toString();
        if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                QStringLiteral("\u786e\u5b9a\u5220\u9664\u8be5\u4efb\u52a1\uff1f")) == QMessageBox::Yes) {
            RunProgramService().remove(uuid);
        }
    } else {
        if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
                QStringLiteral("\u786e\u5b9a\u5220\u9664 %1 \u4e2a\u4efb\u52a1\uff1f").arg(count)) // 确定删除 X 个任务？
            == QMessageBox::Yes) {
            for (int row : rows) {
                RunProgramService().remove(table_->item(row, 0)->data(Qt::UserRole).toString());
            }
        }
    }
    refresh();
    emit dataChanged();
}

void RunProgramPage::testRunSelected() {
    int row = table_->currentRow();
    if (row < 0) return;
    QString uuid = table_->item(row, 0)->data(Qt::UserRole).toString();
    RunProgramService svc;
    auto t = svc.findByUuid(uuid);
    if (t.uuid.isEmpty()) return;
    if (!svc.executeNow(t)) {
        QMessageBox::warning(this, QStringLiteral("\u9519\u8bef"),
            QStringLiteral("\u542f\u52a8\u5931\u8d25\uff0c\u8bf7\u68c0\u67e5\u8def\u5f84")); // 启动失败，请检查路径
    }
}

} // namespace mcclock::gui
