#include "time_calculator_dialog.h"
#include "../widgets/frameless_helper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QMouseEvent>

namespace mcclock::gui {

TimeCalculatorDialog::TimeCalculatorDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("\u65f6\u95f4\u8ba1\u7b97\u5668")); // 时间计算器
    resize(420, 300);

    // Build UI first, THEN apply frameless style
    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    tabs->addTab(createDiffTab(), QStringLiteral("\u65f6\u95f4\u5dee"));     // \u65f6\u95f4\u5dee
    tabs->addTab(createArithmeticTab(), QStringLiteral("\u65f6\u95f4\u52a0\u51cf")); // \u65f6\u95f4\u52a0\u51cf
    layout->addWidget(tabs);

    // Apply frameless style AFTER all controls are created
    FramelessHelper::applyToInlineDialog(this, QStringLiteral("\u65f6\u95f4\u8ba1\u7b97\u5668"));
}

void TimeCalculatorDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    FramelessHelper::showOverlay(parentWidget());
}

void TimeCalculatorDialog::closeEvent(QCloseEvent* event) {
    FramelessHelper::hideOverlay(parentWidget());
    QDialog::closeEvent(event);
}

void TimeCalculatorDialog::reject() {
    FramelessHelper::hideOverlay(parentWidget());
    QDialog::reject();
}

void TimeCalculatorDialog::accept() {
    FramelessHelper::hideOverlay(parentWidget());
    QDialog::accept();
}

QWidget* TimeCalculatorDialog::createDiffTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* form = new QFormLayout();

    diffStart_ = new QDateTimeEdit(QDateTime::currentDateTime(), page);
    diffEnd_ = new QDateTimeEdit(QDateTime::currentDateTime(), page);
    for (auto* e : {diffStart_, diffEnd_}) {
        e->setDisplayFormat("yyyy-MM-dd HH:mm");
        e->setCalendarPopup(true);
    }
    diffEnd_->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    form->addRow(QStringLiteral("\u8d77\u59cb\u65f6\u95f4\uff1a"), diffStart_); // 起始时间：
    form->addRow(QStringLiteral("\u7ed3\u675f\u65f6\u95f4\uff1a"), diffEnd_);   // 结束时间：
    layout->addLayout(form);

    auto* calcBtn = new QPushButton(QStringLiteral("\u8ba1\u7b97"), page); // 计算
    layout->addWidget(calcBtn);

    diffResult_ = new QLabel(page);
    diffResult_->setStyleSheet("font-size: 16px; color: #009688; font-weight: bold;");
    diffResult_->setAlignment(Qt::AlignCenter);
    layout->addWidget(diffResult_, 1);

    connect(calcBtn, &QPushButton::clicked, this, [this]() {
        qint64 secs = diffStart_->dateTime().secsTo(diffEnd_->dateTime());
        bool neg = secs < 0;
        if (neg) secs = -secs;
        qint64 days = secs / 86400;
        qint64 hours = (secs % 86400) / 3600;
        qint64 mins = (secs % 3600) / 60;
        qint64 ss = secs % 60;
        diffResult_->setText((neg ? QStringLiteral("\u76f8\u5dee\uff08\u8d1f\uff09") : QStringLiteral("\u76f8\u5dee")) // 相差（负）/相差
            + QStringLiteral("\uff1a%1 \u5929 %2 \u65f6 %3 \u5206 %4 \u79d2") // ：X 天 X 时 X 分 X 秒
              .arg(days).arg(hours).arg(mins).arg(ss));
    });
    return page;
}

QWidget* TimeCalculatorDialog::createArithmeticTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* form = new QFormLayout();

    baseEdit_ = new QDateTimeEdit(QDateTime::currentDateTime(), page);
    baseEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    baseEdit_->setCalendarPopup(true);
    form->addRow(QStringLiteral("\u57fa\u51c6\u65f6\u95f4\uff1a"), baseEdit_); // 基准时间：

    opCombo_ = new QComboBox(page);
    opCombo_->addItem(QStringLiteral("\u52a0\uff08+\uff09"), 1); // 加（+）
    opCombo_->addItem(QStringLiteral("\u51cf\uff08-\uff09"), -1); // 减（-）
    form->addRow(QStringLiteral("\u8fd0\u7b97\uff1a"), opCombo_); // 运算：

    daysSpin_ = new QSpinBox(page);
    daysSpin_->setRange(0, 9999);
    daysSpin_->setSuffix(QStringLiteral(" \u5929")); // 天
    hoursSpin_ = new QSpinBox(page);
    hoursSpin_->setRange(0, 23);
    hoursSpin_->setSuffix(QStringLiteral(" \u65f6")); // 时
    minutesSpin_ = new QSpinBox(page);
    minutesSpin_->setRange(0, 59);
    minutesSpin_->setSuffix(QStringLiteral(" \u5206")); // 分
    auto* durRow = new QHBoxLayout();
    durRow->addWidget(daysSpin_);
    durRow->addWidget(hoursSpin_);
    durRow->addWidget(minutesSpin_);
    form->addRow(QStringLiteral("\u65f6\u957f\uff1a"), durRow); // 时长：
    layout->addLayout(form);

    auto* calcBtn = new QPushButton(QStringLiteral("\u8ba1\u7b97"), page); // 计算
    layout->addWidget(calcBtn);

    arithResult_ = new QLabel(page);
    arithResult_->setStyleSheet("font-size: 16px; color: #009688; font-weight: bold;");
    arithResult_->setAlignment(Qt::AlignCenter);
    layout->addWidget(arithResult_, 1);

    connect(calcBtn, &QPushButton::clicked, this, [this]() {
        qint64 delta = static_cast<qint64>(daysSpin_->value()) * 86400
                     + hoursSpin_->value() * 3600
                     + minutesSpin_->value() * 60;
        delta *= opCombo_->currentData().toInt();
        QDateTime result = baseEdit_->dateTime().addSecs(delta);
        arithResult_->setText(QStringLiteral("\u7ed3\u679c\uff1a%1").arg(result.toString("yyyy-MM-dd HH:mm"))); // 结果：
    });
    return page;
}

void TimeCalculatorDialog::mousePressEvent(QMouseEvent* event) {
    if (!framelessMousePress(this, event))
        QDialog::mousePressEvent(event);
}

void TimeCalculatorDialog::mouseMoveEvent(QMouseEvent* event) {
    if (!framelessMouseMove(this, event))
        QDialog::mouseMoveEvent(event);
}

void TimeCalculatorDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (!framelessMouseRelease(this, event))
        QDialog::mouseReleaseEvent(event);
}

} // namespace mcclock::gui
