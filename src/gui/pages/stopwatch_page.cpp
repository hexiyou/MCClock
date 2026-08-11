#include "stopwatch_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTimer>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

namespace mcclock::gui {

using mcclock::services::StopwatchService;

StopwatchPage::StopwatchPage(QWidget* parent)
    : QWidget(parent)
{
    stopwatch_ = new StopwatchService(this);
    setupUi();

    displayTimer_ = new QTimer(this);
    displayTimer_->setInterval(50);
    connect(displayTimer_, &QTimer::timeout, this, &StopwatchPage::onDisplayTick);
    updateButtons();
}

void StopwatchPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(12);

    display_ = new QLabel("00:00:00.00", this);
    display_->setAlignment(Qt::AlignCenter);
    display_->setStyleSheet("font-size: 56px; font-weight: bold; color: #37474F;"
                            " font-family: 'Consolas', 'Courier New', monospace;");
    root->addWidget(display_);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    startPauseBtn_ = new QPushButton(QStringLiteral("\u5f00\u59cb"), this); // 开始
    auto* lapBtn = lapBtn_ = new QPushButton(QStringLiteral("\u8bb0\u5708"), this); // 记圈
    auto* resetBtn = new QPushButton(QStringLiteral("\u91cd\u7f6e"), this);  // 重置
    auto* copyBtn = new QPushButton(QStringLiteral("\u590d\u5236"), this);   // 复制
    auto* saveBtn = new QPushButton(QStringLiteral("\u4fdd\u5b58"), this);   // 保存
    lapBtn->setProperty("flatStyle", "secondary");
    resetBtn->setProperty("flatStyle", "secondary");
    copyBtn->setProperty("flatStyle", "secondary");
    saveBtn->setProperty("flatStyle", "secondary");
    startPauseBtn_->setMinimumWidth(100);
    btnRow->addWidget(startPauseBtn_);
    btnRow->addWidget(lapBtn);
    btnRow->addWidget(resetBtn);
    btnRow->addWidget(copyBtn);
    btnRow->addWidget(saveBtn);
    btnRow->addStretch();
    root->addLayout(btnRow);

    lapList_ = new QListWidget(this);
    root->addWidget(lapList_, 1);

    connect(startPauseBtn_, &QPushButton::clicked, this, &StopwatchPage::onStartPause);
    connect(lapBtn, &QPushButton::clicked, this, &StopwatchPage::onLap);
    connect(resetBtn, &QPushButton::clicked, this, &StopwatchPage::onReset);
    connect(copyBtn, &QPushButton::clicked, this, &StopwatchPage::onCopy);
    connect(saveBtn, &QPushButton::clicked, this, &StopwatchPage::onSave);
}

QString StopwatchPage::formatMs(qint64 ms) {
    qint64 h = ms / 3600000;
    qint64 m = (ms % 3600000) / 60000;
    qint64 s = (ms % 60000) / 1000;
    qint64 cs = (ms % 1000) / 10;
    return QStringLiteral("%1:%2:%3.%4")
        .arg(h, 2, 10, QLatin1Char('0'))
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'))
        .arg(cs, 2, 10, QLatin1Char('0'));
}

void StopwatchPage::updateButtons() {
    bool running = stopwatch_->state() == StopwatchService::State::Running;
    bool active = stopwatch_->state() != StopwatchService::State::Stopped;
    startPauseBtn_->setText(running ? QStringLiteral("\u6682\u505c")   // 暂停
                                    : (active ? QStringLiteral("\u7ee7\u7eed")  // 继续
                                              : QStringLiteral("\u5f00\u59cb"))); // 开始
    lapBtn_->setEnabled(running);
}

void StopwatchPage::onStartPause() {
    if (stopwatch_->state() == StopwatchService::State::Running) {
        stopwatch_->pause();
        displayTimer_->stop();
    } else {
        if (stopwatch_->state() == StopwatchService::State::Stopped) {
            lapList_->clear();
        }
        stopwatch_->start();
        displayTimer_->start();
    }
    updateButtons();
}

void StopwatchPage::onReset() {
    stopwatch_->reset();
    displayTimer_->stop();
    display_->setText("00:00:00.00");
    lapList_->clear();
    updateButtons();
}

void StopwatchPage::onLap() {
    qint64 total = stopwatch_->lap();
    int index = stopwatch_->laps().size();
    qint64 prev = index >= 2 ? stopwatch_->laps()[index - 2] : 0;
    lapList_->insertItem(0, QStringLiteral("\u7b2c %1 \u5708  %2  \uff08\u672c\u5708 %3\uff09")
        .arg(index).arg(formatMs(total)).arg(formatMs(total - prev))); // 第 N 圈  总计（本圈 X）
}

void StopwatchPage::onCopy() {
    QStringList lines;
    lines << QStringLiteral("\u603b\u8ba1\u65f6\uff1a%1").arg(formatMs(stopwatch_->elapsedMs())); // 总计时：
    auto laps = stopwatch_->laps();
    for (int i = 0; i < laps.size(); ++i) {
        lines << QStringLiteral("\u7b2c %1 \u5708\uff1a%2").arg(i + 1).arg(formatMs(laps[i])); // 第 N 圈：
    }
    QApplication::clipboard()->setText(lines.join('\n'));
}

void StopwatchPage::onSave() {
    QString path = QFileDialog::getSaveFileName(this,
        QStringLiteral("\u4fdd\u5b58\u8ba1\u65f6\u8bb0\u5f55"), // 保存计时记录
        QString(), QStringLiteral("Text (*.txt)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("\u9519\u8bef"),
            QStringLiteral("\u4fdd\u5b58\u5931\u8d25")); // 保存失败
        return;
    }
    QTextStream out(&f);
    out << QStringLiteral("\u603b\u8ba1\u65f6\uff1a%1\n").arg(formatMs(stopwatch_->elapsedMs()));
    auto laps = stopwatch_->laps();
    for (int i = 0; i < laps.size(); ++i) {
        out << QStringLiteral("\u7b2c %1 \u5708\uff1a%2\n").arg(i + 1).arg(formatMs(laps[i]));
    }
}

void StopwatchPage::onDisplayTick() {
    display_->setText(formatMs(stopwatch_->elapsedMs()));
}

} // namespace mcclock::gui
