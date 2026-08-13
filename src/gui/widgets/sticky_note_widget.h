#pragma once

#include <QWidget>
#include <QPoint>
#include <QSize>
#include <QColor>

class QTextEdit;
class QLabel;
class QPushButton;

namespace mcclock::gui {

// Semi-transparent sticky note window; content persisted to JSON
// (sticky_notes.json in the app data directory).
class StickyNoteWidget : public QWidget {
    Q_OBJECT
public:
    explicit StickyNoteWidget(QWidget* parent = nullptr);

signals:
    void visibilityChanged(bool visible);

protected:
    void paintEvent(QPaintEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void closeEvent(QCloseEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

private slots:
    void saveToFile();
    void showThemePopup();
    void applyThemeColor(const QColor& color);

private:
    void load();
    void save();
    void updateEditorFont();
    void applyOpacity(int alpha);
    void showEditorContextMenu(const QPoint& pos);
    Qt::Edges edgeAtPos(const QPoint& pos) const;
    void updateCursorForEdge(Qt::Edges edges);
    QString makeBtnStyleSheet(const QString& bg);

    QTextEdit* editor_ = nullptr;
    QLabel* closeBtn_ = nullptr;
    QPushButton* saveBtn_ = nullptr;
    QPushButton* themeBtn_ = nullptr;
    QColor bgColor_ = QColor(255, 249, 196, 240);
    int opacity_ = 240;          // alpha 0-255
    QString fontFamily_;         // empty = default
    int fontSize_ = 14;
    QPoint dragPos_;
    bool dragging_ = false;
    bool resizing_ = false;
    Qt::Edges resizeEdges_ = Qt::Edges();
    QPoint resizePressPos_;
    QSize resizeOrigSize_;
    int edgeMargin_ = 8;
};

} // namespace mcclock::gui
