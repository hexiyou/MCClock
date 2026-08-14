#pragma once

#include <QDialog>

class QDateTimeEdit;
class QSpinBox;
class QComboBox;
class QLabel;
class QMouseEvent;
class QShowEvent;
class QCloseEvent;

namespace mcclock::gui {

// Time calculator: two tabs - time difference and time add/subtract.
class TimeCalculatorDialog : public QDialog {
    Q_OBJECT
public:
    explicit TimeCalculatorDialog(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

public slots:
    void reject() override;
    void accept() override;

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
