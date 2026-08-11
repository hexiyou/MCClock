#include "sidebar_widget.h"
#include "sticky_note_widget.h"
#include "dialogs/time_calculator_dialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QDialog>
#include <QCalendarWidget>
#include <QProcess>
#include <QLabel>

namespace mcclock::gui {

SidebarWidget::SidebarWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("Sidebar");
    setFixedWidth(72);
    setStyleSheet("#Sidebar { background: #ECEFF1; }");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 10, 6, 10);
    layout->setSpacing(10);

    auto makeBtn = [this, layout](const QString& text) {
        auto* btn = new QPushButton(text, this);
        btn->setFixedHeight(52);
        btn->setProperty("flatStyle", "secondary");
        btn->setStyleSheet("font-size: 12px;");
        layout->addWidget(btn);
        return btn;
    };

    auto* calendarBtn = makeBtn(QStringLiteral("\u65e5\u5386"));         // 日历
    auto* noteBtn = makeBtn(QStringLiteral("\u4fbf\u7b7e"));            // 便签
    auto* timeCalcBtn = makeBtn(QStringLiteral("\u65f6\u95f4\n\u8ba1\u7b97")); // 时间计算
    auto* sysCalcBtn = makeBtn(QStringLiteral("\u8ba1\u7b97\u5668"));   // 计算器
    clockToggleBtn_ = makeBtn(QStringLiteral("\u684c\u9762\n\u65f6\u949f"));   // 桌面时钟
    clockToggleBtn_->setCheckable(true);
    layout->addStretch();

    connect(calendarBtn, &QPushButton::clicked, this, &SidebarWidget::openCalendar);
    connect(noteBtn, &QPushButton::clicked, this, &SidebarWidget::openStickyNote);
    connect(timeCalcBtn, &QPushButton::clicked, this, &SidebarWidget::openTimeCalculator);
    connect(sysCalcBtn, &QPushButton::clicked, this, &SidebarWidget::openSystemCalculator);
    connect(clockToggleBtn_, &QPushButton::toggled, this, &SidebarWidget::desktopClockToggled);
}

void SidebarWidget::openCalendar() {
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("\u65e5\u5386")); // 日历
    auto* layout = new QVBoxLayout(&dlg);
    auto* cal = new QCalendarWidget(&dlg);
    cal->setGridVisible(true);
    cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    layout->addWidget(cal);
    auto* label = new QLabel(&dlg);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: #546E7A;");
    label->setText(cal->selectedDate().toString("yyyy-MM-dd dddd"));
    layout->addWidget(label);
    connect(cal, &QCalendarWidget::selectionChanged, &dlg, [cal, label]() {
        label->setText(cal->selectedDate().toString("yyyy-MM-dd dddd"));
    });
    dlg.resize(420, 360);
    dlg.exec();
}

void SidebarWidget::openStickyNote() {
    if (!note_) {
        note_ = new StickyNoteWidget(nullptr);
        connect(note_, &QObject::destroyed, this, [this]() { note_ = nullptr; });
    }
    note_->show();
    note_->raise();
    note_->activateWindow();
}

void SidebarWidget::openTimeCalculator() {
    TimeCalculatorDialog dlg(this);
    dlg.exec();
}

void SidebarWidget::openSystemCalculator() {
    QProcess::startDetached("calc.exe", {});
}

} // namespace mcclock::gui
