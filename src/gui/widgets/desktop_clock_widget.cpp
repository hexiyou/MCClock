#include "desktop_clock_widget.h"
#include "core/services/lunar_calendar.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QDateTime>
#include <QMenu>
#include <QActionGroup>
#include <QContextMenuEvent>

namespace mcclock::gui {

using mcclock::services::LunarCalendar;

DesktopClockWidget::DesktopClockWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(220, 84);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 8, 14, 8);
    layout->setSpacing(0);

    timeLabel_ = new QLabel(this);
    timeLabel_->setAlignment(Qt::AlignCenter);
    dateLabel_ = new QLabel(this);
    dateLabel_->setAlignment(Qt::AlignCenter);
    setSize(1); // medium by default
    layout->addWidget(timeLabel_);
    layout->addWidget(dateLabel_);

    auto updateClock = [this]() {
        QDateTime now = QDateTime::currentDateTime();
        timeLabel_->setText(now.toString("HH:mm:ss"));
        auto lunar = LunarCalendar::solarToLunar(now.date().year(), now.date().month(), now.date().day());
        dateLabel_->setText(QStringLiteral("%1  %2%3")
            .arg(now.toString("yyyy-MM-dd ddd"))
            .arg(LunarCalendar::getLunarMonthName(lunar.month, lunar.isLeapMonth))
            .arg(LunarCalendar::getLunarDayName(lunar.day)));
    };
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, updateClock);
    timer->start(1000);
    updateClock();

    // Default position: top-right corner of primary screen
    if (auto* screen = QGuiApplication::primaryScreen()) {
        QRect geo = screen->availableGeometry();
        move(geo.right() - width() - 20, geo.top() + 20);
    }
}

void DesktopClockWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        dragPos_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    }
}

void DesktopClockWidget::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        move(e->globalPosition().toPoint() - dragPos_);
        e->accept();
    }
}

void DesktopClockWidget::setSize(int size) {
    size_ = qBound(0, size, 2);
    static const struct { int w, h, timePx, datePx; } kSizes[] = {
        {300, 112, 46, 15}, // large
        {220, 84, 34, 13},  // medium
        {180, 64, 24, 11},  // small
    };
    const auto& d = kSizes[size_];
    setFixedSize(d.w, d.h);
    timeLabel_->setStyleSheet(QStringLiteral(
        "color: #FFFFFF; font-size: %1px; font-weight: bold;"
        " background: rgba(38, 50, 56, 170); border-radius: 8px 8px 0px 0px;").arg(d.timePx));
    dateLabel_->setStyleSheet(QStringLiteral(
        "color: #ECEFF1; font-size: %1px;"
        " background: rgba(38, 50, 56, 170); border-radius: 0px 0px 8px 8px;").arg(d.datePx));
}

void DesktopClockWidget::contextMenuEvent(QContextMenuEvent* e) {
    QMenu menu;
    auto* showMain = menu.addAction(QStringLiteral("\u663e\u793a\u8f6f\u4ef6\u4e3b\u754c\u9762")); // 显示软件主界面
    auto* closeClock = menu.addAction(QStringLiteral("\u5173\u95ed\u684c\u9762\u65f6\u949f"));   // 关闭桌面时钟

    // 窗口尺寸 submenu: 大 / 中 / 小
    auto* sizeMenu = menu.addMenu(QStringLiteral("\u7a97\u53e3\u5c3a\u5bf8"));
    auto* group = new QActionGroup(sizeMenu);
    const QStringList sizeNames = {
        QStringLiteral("\u5927"), // 大
        QStringLiteral("\u4e2d"), // 中
        QStringLiteral("\u5c0f"), // 小
    };
    for (int i = 0; i < sizeNames.size(); ++i) {
        auto* a = sizeMenu->addAction(sizeNames[i]);
        a->setCheckable(true);
        a->setChecked(i == size_);
        group->addAction(a);
        connect(a, &QAction::triggered, this, [this, i]() {
            setSize(i);
            emit sizeChanged(i);
        });
    }

    connect(showMain, &QAction::triggered, this, &DesktopClockWidget::showMainWindowRequested);
    connect(closeClock, &QAction::triggered, this, &DesktopClockWidget::closeRequested);
    menu.exec(e->globalPos());
    e->accept();
}

HourlyChimePopup::HourlyChimePopup(int hour, QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedSize(240, 64);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* label = new QLabel(QStringLiteral("\u73b0\u5728\u662f %1 \u70b9\u6574").arg(hour), this); // 现在是 N 点整
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: #FFFFFF; font-size: 22px; font-weight: bold;"
                         " background: rgba(33, 150, 243, 220); border-radius: 10px;");
    layout->addWidget(label);

    if (auto* screen = QGuiApplication::primaryScreen()) {
        QRect geo = screen->availableGeometry();
        move(geo.center().x() - width() / 2, geo.top() + 60);
    }

    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &HourlyChimePopup::close);
    timer->start(5000);
    show();
}

} // namespace mcclock::gui
