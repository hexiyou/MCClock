#pragma once

#include <QWidget>

class QLabel;

namespace mcclock::gui {

// Always-on-top reminder popup shown when an alarm/birthday triggers.
// Position respects settings reminder_position (center/left_up/left_down).
class ReminderPopup : public QWidget {
    Q_OBJECT
public:
    explicit ReminderPopup(const QString& title, const QString& message,
                           QWidget* parent = nullptr);

    // Position according to settings and show
    void showAtConfiguredPosition();

signals:
    void dismissClicked();

private:
    void setupUi(const QString& title, const QString& message);
};

} // namespace mcclock::gui
