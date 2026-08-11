#pragma once

#include <QDialog>

class QRadioButton;
class QCheckBox;

namespace mcclock::gui {

// Close confirmation dialog (300x190):
//   option 1: minimize to system tray (default)
//   option 2: exit program
//   checkbox: don't show again
class CloseConfirmDialog : public QDialog {
    Q_OBJECT
public:
    enum Action { MinimizeToTray = 0, ExitProgram = 1 };

    explicit CloseConfirmDialog(QWidget* parent = nullptr);

    Action selectedAction() const;
    bool dontAskAgain() const;

private:
    QRadioButton* trayOption_ = nullptr;
    QRadioButton* exitOption_ = nullptr;
    QCheckBox* dontAskCheck_ = nullptr;
};

} // namespace mcclock::gui
