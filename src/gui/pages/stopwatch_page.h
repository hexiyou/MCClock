#pragma once

#include <QWidget>
#include "core/services/scheduler.h"

class QLabel;
class QPushButton;
class QListWidget;
class QTimer;

namespace mcclock::gui {

// Stopwatch page: big elapsed-time display, lap recording,
// copy-to-clipboard and save-to-file support.
class StopwatchPage : public QWidget {
    Q_OBJECT
public:
    explicit StopwatchPage(QWidget* parent = nullptr);

private slots:
    void onStartPause();
    void onReset();
    void onLap();
    void onCopy();
    void onSave();
    void onDisplayTick();

private:
    void setupUi();
    void updateButtons();
    static QString formatMs(qint64 ms);

    mcclock::services::StopwatchService* stopwatch_ = nullptr;
    QTimer* displayTimer_ = nullptr;
    QLabel* display_ = nullptr;
    QPushButton* startPauseBtn_ = nullptr;
    QPushButton* lapBtn_ = nullptr;
    QListWidget* lapList_ = nullptr;
};

} // namespace mcclock::gui
