#pragma once

#include <QWidget>
#include "core/models/all_models.h"

class QCheckBox;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QTimer;

namespace mcclock::services { class RingtoneManager; }

namespace mcclock::gui {

// Health reminder page: work/rest cycle with enable switch,
// fullscreen/window display mode configuration.
class HealthPage : public QWidget {
    Q_OBJECT
public:
    explicit HealthPage(QWidget* parent = nullptr);

private slots:
    void onSave();
    void onToggleSession();
    void onTick();

private:
    void setupUi();
    void loadSettings();
    void showPhaseReminder(bool restPhase);

    QCheckBox* enableCheck_ = nullptr;
    QComboBox* displayCombo_ = nullptr;
    QSpinBox* workSpin_ = nullptr;
    QSpinBox* restSpin_ = nullptr;
    QLineEdit* labelEdit_ = nullptr;
    QPushButton* toggleBtn_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    QTimer* sessionTimer_ = nullptr;
    mcclock::services::RingtoneManager* ringtone_ = nullptr;
    models::HealthSettings settings_;
    bool inRestPhase_ = false;
    int phaseSecondsLeft_ = 0;
};

} // namespace mcclock::gui
