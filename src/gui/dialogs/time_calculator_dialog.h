#pragma once

#include <QDialog>

class QDateTimeEdit;
class QSpinBox;
class QComboBox;
class QLabel;

namespace mcclock::gui {

// Time calculator: two tabs - time difference and time add/subtract.
class TimeCalculatorDialog : public QDialog {
    Q_OBJECT
public:
    explicit TimeCalculatorDialog(QWidget* parent = nullptr);

private:
    QWidget* createDiffTab();
    QWidget* createArithmeticTab();

    // Diff tab
    QDateTimeEdit* diffStart_ = nullptr;
    QDateTimeEdit* diffEnd_ = nullptr;
    QLabel* diffResult_ = nullptr;

    // Arithmetic tab
    QDateTimeEdit* baseEdit_ = nullptr;
    QSpinBox* daysSpin_ = nullptr;
    QSpinBox* hoursSpin_ = nullptr;
    QSpinBox* minutesSpin_ = nullptr;
    QComboBox* opCombo_ = nullptr;
    QLabel* arithResult_ = nullptr;
};

} // namespace mcclock::gui
