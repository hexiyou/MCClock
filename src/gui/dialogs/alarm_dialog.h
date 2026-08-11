#pragma once

#include <QDialog>
#include <QList>
#include "core/models/all_models.h"

class QCheckBox;
class QComboBox;
class QSpinBox;
class QTimeEdit;
class QDateEdit;
class QLineEdit;
class QStackedWidget;
class QLabel;

namespace mcclock::gui {

// Add/edit alarm dialog: cycle mode with dynamic UI, ringtone and ring mode.
class AlarmDialog : public QDialog {
    Q_OBJECT
public:
    // Pass existing alarm to edit; default-constructed alarm to add.
    explicit AlarmDialog(const mcclock::models::Alarm& existing = {},
                         QWidget* parent = nullptr);

    mcclock::models::Alarm alarm() const { return alarm_; }

private slots:
    void onCycleModeChanged(int index);
    void previewRingtone();
    void save();

private:
    void setupUi();
    void loadFromModel();
    QString buildCycleData() const;

    mcclock::models::Alarm alarm_;
    bool editing_ = false;

    QCheckBox* enabledCheck_ = nullptr;
    QComboBox* cycleCombo_ = nullptr;
    QStackedWidget* cycleStack_ = nullptr;
    // Cycle option widgets
    QDateEdit* onceDateEdit_ = nullptr;
    QList<QCheckBox*> weekdayChecks_;
    QSpinBox* monthDaySpin_ = nullptr;
    QSpinBox* yearMonthSpin_ = nullptr;
    QSpinBox* yearDaySpin_ = nullptr;
    QSpinBox* intervalSpin_ = nullptr;

    QTimeEdit* timeEdit_ = nullptr;
    QCheckBox* rangeCheck_ = nullptr;
    QDateEdit* rangeStartEdit_ = nullptr;
    QDateEdit* rangeEndEdit_ = nullptr;

    QComboBox* ringtoneCombo_ = nullptr;
    QLineEdit* customPathEdit_ = nullptr;
    QComboBox* ringModeCombo_ = nullptr;
    QSpinBox* customMinutesSpin_ = nullptr;
    QLineEdit* labelEdit_ = nullptr;
};

} // namespace mcclock::gui
