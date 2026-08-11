#pragma once

#include <QWidget>
#include <QPoint>

class QTextEdit;

namespace mcclock::gui {

// Semi-transparent sticky note window; content persisted to JSON
// (sticky_notes.json in the app data directory).
class StickyNoteWidget : public QWidget {
    Q_OBJECT
public:
    explicit StickyNoteWidget(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private:
    void load();
    void save();

    QTextEdit* editor_ = nullptr;
    QPoint dragPos_;
    bool dragging_ = false;
};

} // namespace mcclock::gui
