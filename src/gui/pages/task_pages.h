#pragma once

#include <QWidget>
#include "core/models/all_models.h"

class QTableWidget;
class QPushButton;

namespace mcclock::gui {

// Scheduled shutdown page: task list + add/edit/delete.
// Edit dialog is embedded (simple modal form).
class ShutdownPage : public QWidget {
    Q_OBJECT
public:
    explicit ShutdownPage(QWidget* parent = nullptr);
    void refresh();

signals:
    void dataChanged();

private slots:
    void addTask();
    void editSelected();
    void deleteSelected();
    void executeSelected();
    void onHeaderDoubleClicked(int logicalIndex);

private:
    void setupUi();

    QTableWidget* table_ = nullptr;
    int sortColumn_ = -1;
    Qt::SortOrder sortOrder_ = Qt::AscendingOrder;
};

// Run program page: task list with program path + test-run.
class RunProgramPage : public QWidget {
    Q_OBJECT
public:
    explicit RunProgramPage(QWidget* parent = nullptr);
    void refresh();

signals:
    void dataChanged();

private slots:
    void addTask();
    void editSelected();
    void deleteSelected();
    void testRunSelected();
    void copySelected();
    void onHeaderDoubleClicked(int logicalIndex);

private:
    void setupUi();

    QTableWidget* table_ = nullptr;
    int sortColumn_ = -1;
    Qt::SortOrder sortOrder_ = Qt::AscendingOrder;
};

} // namespace mcclock::gui
