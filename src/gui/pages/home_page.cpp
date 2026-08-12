#include "home_page.h"
#include "core/services/lunar_calendar.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

namespace mcclock::gui {

using mcclock::services::LunarCalendar;

HomePage::HomePage(QWidget* parent)
    : QWidget(parent)
{
    startTime_ = QDateTime::currentDateTime();

    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);

    timeLabel_ = new QLabel(this);
    timeLabel_->setAlignment(Qt::AlignCenter);
    timeLabel_->setStyleSheet("font-size: 56px; font-weight: bold; color: #1E88E5;");
    layout->addWidget(timeLabel_);

    dateLabel_ = new QLabel(this);
    dateLabel_->setAlignment(Qt::AlignCenter);
    dateLabel_->setStyleSheet("font-size: 18px; color: #37474F;");
    layout->addWidget(dateLabel_);

    lunarLabel_ = new QLabel(this);
    lunarLabel_->setAlignment(Qt::AlignCenter);
    lunarLabel_->setStyleSheet("font-size: 15px; color: #78909C;");
    layout->addWidget(lunarLabel_);

    extraLabel_ = new QLabel(this);
    extraLabel_->setAlignment(Qt::AlignCenter);
    extraLabel_->setStyleSheet("font-size: 13px; color: #90A4AE;");
    layout->addWidget(extraLabel_);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &HomePage::updateClock);
    timer_->start(1000);
    updateClock();
}

void HomePage::updateThemeColor(const QColor& primaryColor) {
    timeLabel_->setStyleSheet(QString("font-size: 56px; font-weight: bold; color: %1;").arg(primaryColor.name()));
}

void HomePage::updateClock() {
    QDateTime now = QDateTime::currentDateTime();
    QDate d = now.date();

    timeLabel_->setText(now.toString("HH:mm:ss"));

    static const char* weekNames[] = {
        "\xe5\x91\xa8\xe4\xb8\x80", "\xe5\x91\xa8\xe4\xba\x8c", "\xe5\x91\xa8\xe4\xb8\x89",
        "\xe5\x91\xa8\xe5\x9b\x9b", "\xe5\x91\xa8\xe4\xba\x94", "\xe5\x91\xa8\xe5\x85\xad",
        "\xe5\x91\xa8\xe6\x97\xa5"
    };
    dateLabel_->setText(QStringLiteral("%1\u5e74%2\u6708%3\u65e5 %4")
        .arg(d.year()).arg(d.month()).arg(d.day())
        .arg(QString::fromUtf8(weekNames[d.dayOfWeek() - 1])));

    // Lunar info
    auto lunar = LunarCalendar::solarToLunar(d.year(), d.month(), d.day());
    QString lunarText = QStringLiteral("\u519c\u5386 %1\u5e74%2%3")
        .arg(lunar.year)
        .arg(LunarCalendar::getLunarMonthName(lunar.month, lunar.isLeapMonth))
        .arg(LunarCalendar::getLunarDayName(lunar.day));
    lunarLabel_->setText(lunarText);

    // Zodiac + constellation + uptime
    qint64 uptimeSecs = startTime_.secsTo(now);
    QString uptime = QStringLiteral("%1\u5c0f\u65f6%2\u5206%3\u79d2") // X小时X分X秒
        .arg(uptimeSecs / 3600).arg((uptimeSecs % 3600) / 60).arg(uptimeSecs % 60);
    extraLabel_->setText(QStringLiteral("\u751f\u8096\uff1a%1    \u661f\u5ea7\uff1a%2    \u672c\u6b21\u8fd0\u884c\uff1a%3")
        .arg(LunarCalendar::getZodiacAnimal(lunar.year))
        .arg(LunarCalendar::getConstellation(d.month(), d.day()))
        .arg(uptime));
}

} // namespace mcclock::gui
