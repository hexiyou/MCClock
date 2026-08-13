#pragma once

#include <QWidget>
#include "core/models/all_models.h"

class QTableWidget;
class QPushButton;
class QTimer;

namespace mcclock::services { class RingtoneManager; }

namespace mcclock::gui {

// Countdown page: relative (duration) and absolute (target datetime) modes.
// Running countdowns tick locally and persist remaining seconds.
class CountdownPage : public QWidget {
    Q_OBJECT
public:
    explicit CountdownPage(QWidget* parent = nullptr);
    void refresh();

signals:
    void dataChanged();

private slots:
    void addCountdown();
    void editSelected();
    void deleteSelected();
    void startStopSelected();
    void resetSelected();
    void onTick();
    void onHeaderDoubleClicked(int logicalIndex);

private:
    void setupUi();
    void startCountdown(const QString& uuid);
    void finishCountdown(models::Countdown c);
    models::Countdown currentSelected();

    QTableWidget* table_ = nullptr;
    QTimer* tickTimer_ = nullptr;
    mcclock::services::RingtoneManager* ringtone_ = nullptr;
    int sortColumn_ = -1;
    Qt::SortOrder sortOrder_ = Qt::AscendingOrder;
};

} // namespace mcclock::gui
