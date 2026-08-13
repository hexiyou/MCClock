#pragma once

#include <QWidget>
#include "core/models/all_models.h"

class QTableWidget;
class QComboBox;
class QPushButton;

namespace mcclock::gui {

// Alarm list page: table + toolbar (add/edit/delete/recycle bin/sort/filter)
class AlarmPage : public QWidget {
    Q_OBJECT
public:
    explicit AlarmPage(QWidget* parent = nullptr);

    void refresh();

signals:
    // Emitted when alarm data changed so scheduler cache can reload
    void dataChanged();

private slots:
    void addAlarm();
    void editSelected();
    void deleteSelected();
    void restoreSelected();
    void copySelected();
    void toggleRecycleBinView();
    void clearRecycleBin();
    void onCellDoubleClicked(int row, int column);
    void onEnableToggled(int row);

private:
    void setupUi();
    void refreshGroupCombo();
    void createGroup();
    void manageGroups();
    QString cycleDescription(const mcclock::models::Alarm& a) const;

    QTableWidget* table_ = nullptr;
    QComboBox* groupCombo_ = nullptr;
    QComboBox* sortCombo_ = nullptr;
    QPushButton* editBtn_ = nullptr;
    QPushButton* copyBtn_ = nullptr;
    QPushButton* recycleBinBtn_ = nullptr;
    QPushButton* restoreBtn_ = nullptr;
    QPushButton* clearRecycleBtn_ = nullptr;
    bool recycleBinView_ = false;
};

} // namespace mcclock::gui
