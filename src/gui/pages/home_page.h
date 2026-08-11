#pragma once

#include <QWidget>
#include <QDateTime>

class QLabel;
class QTimer;

namespace mcclock::gui {

// Home page: large realtime clock, date, lunar calendar info,
// zodiac/constellation, and app uptime.
class HomePage : public QWidget {
    Q_OBJECT
public:
    explicit HomePage(QWidget* parent = nullptr);

private slots:
    void updateClock();

private:
    QLabel* timeLabel_ = nullptr;
    QLabel* dateLabel_ = nullptr;
    QLabel* lunarLabel_ = nullptr;
    QLabel* extraLabel_ = nullptr;   // zodiac + constellation + uptime
    QTimer* timer_ = nullptr;
    QDateTime startTime_;
};

} // namespace mcclock::gui
