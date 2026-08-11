#pragma once

#include <QWidget>

class QLabel;

namespace mcclock::gui {

// Frameless, translucent, draggable desktop floating clock.
class DesktopClockWidget : public QWidget {
    Q_OBJECT
public:
    explicit DesktopClockWidget(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;

private:
    QLabel* timeLabel_ = nullptr;
    QLabel* dateLabel_ = nullptr;
    QPoint dragPos_;
};

// Small topmost popup shown on the hourly chime; auto-hides after seconds.
class HourlyChimePopup : public QWidget {
    Q_OBJECT
public:
    explicit HourlyChimePopup(int hour, QWidget* parent = nullptr);
};

} // namespace mcclock::gui
