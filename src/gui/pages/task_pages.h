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

private:
    void setupUi();

    QTableWidget* table_ = nullptr;
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

private:
    void setupUi();

    QTableWidget* table_ = nullptr;
};

} // namespace mcclock::gui
