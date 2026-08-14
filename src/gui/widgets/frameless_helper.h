#pragma once

#include <QWidget>
#include <QPoint>
#include <QObject>

class QMouseEvent;
class QHBoxLayout;
class QDialog;
class QWidget;
class QEvent;

namespace mcclock::gui {

// Event filter for handling drag on inline dialogs
class FramelessDragFilter : public QObject {
    Q_OBJECT
public:
    explicit FramelessDragFilter(QObject* parent = nullptr);
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
private:
    bool dragging_ = false;
    QPoint dragPos_;
};

// Lightweight frameless window helper: flat title bar, drag, resize edges.
// Usage: call FramelessHelper::setup(mainWindow) once after setCentralWidget.
class FramelessHelper {
public:
    static QWidget* setup(QWidget* window, const QString& title = QString());
    static QWidget* setupDialog(QDialog* dialog, const QString& title = QString());
    static void setMaximizeEnabled(QWidget* window, bool enabled);

    // Helper: create a title bar widget for inline dialogs
    // Call AFTER all content widgets are created, then wrap the dialog layout
    static void applyToInlineDialog(QDialog* dialog, const QString& title);
    
    // Show overlay shadow on parent widget when dialog is shown
    static void showOverlay(QWidget* parent);
    static void hideOverlay(QWidget* parent);
};

// Mouse event handlers — call from MainWindow mousePressEvent/MoveEvent/ReleaseEvent.
bool framelessMousePress(QWidget* w, QMouseEvent* e);
bool framelessMouseMove(QWidget* w, QMouseEvent* e);
bool framelessMouseRelease(QWidget* w, QMouseEvent* e);

} // namespace mcclock::gui
