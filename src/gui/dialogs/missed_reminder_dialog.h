#pragma once

#include <QDialog>
#include <QList>
#include "core/models/all_models.h"

class QListWidget;

namespace mcclock::gui {

// Dialog shown at startup listing alarms that fired while the app was closed.
class MissedReminderDialog : public QDialog {
    Q_OBJECT
public:
    explicit MissedReminderDialog(const QList<models::Alarm>& missed,
                                  QWidget* parent = nullptr);

private:
    void setupUi(const QList<models::Alarm>& missed);
};

} // namespace mcclock::gui
