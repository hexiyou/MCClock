#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QTimeEdit;
class QSlider;
class QLineEdit;

namespace mcclock::gui {

// Global settings dialog with 4 tabs:
//   1. 基本设置  - auto start, missed reminder, check update
//   2. 提醒设置  - fullscreen mode/period, reminder position, close mode, volume
//   3. 整点报时  - chime mode/cycle/custom hours
//   4. 高级设置  - HTTP API server (disabled by default), login/register stub
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

signals:
    // Emitted after save so MainWindow can restart API server etc.
    void settingsSaved();

private:
    QWidget* createGeneralTab();
    QWidget* createReminderTab();
    QWidget* createChimeTab();
    QWidget* createAdvancedTab();
    QWidget* createAboutTab();
    void loadSettings();
    void saveSettings();
    void exportAllData();
    void importAllData();

    // Tab 1
    QCheckBox* autoStartCheck_ = nullptr;
    QCheckBox* missedCheck_ = nullptr;
    QCheckBox* autoUpdateCheck_ = nullptr;
    QCheckBox* desktopClockCheck_ = nullptr;

    // Tab 2
    QCheckBox* fullscreenCheck_ = nullptr;
    QTimeEdit* fullscreenStart_ = nullptr;
    QTimeEdit* fullscreenEnd_ = nullptr;
    QComboBox* positionCombo_ = nullptr;
    QComboBox* closeModeCombo_ = nullptr;
    QSpinBox* autoCloseSpin_ = nullptr;
    QSlider* volumeSlider_ = nullptr;

    // Tab 3
    QComboBox* chimeModeCombo_ = nullptr;
    QComboBox* chimeCycleCombo_ = nullptr;
    QList<QCheckBox*> chimeHourChecks_;

    // Tab 4
    QCheckBox* apiEnabledCheck_ = nullptr;
    QLineEdit* apiIpEdit_ = nullptr;
    QSpinBox* apiPortSpin_ = nullptr;
};

} // namespace mcclock::gui
