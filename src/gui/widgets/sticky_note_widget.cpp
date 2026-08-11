#include "sticky_note_widget.h"
#include "core/utils/platform_utils.h"

#include <QVBoxLayout>
#include <QTextEdit>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace mcclock::gui {

namespace {

QString noteFilePath() {
    return mcclock::utils::PlatformUtils::appDataPath() + "/sticky_notes.json";
}

} // namespace

StickyNoteWidget::StickyNoteWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(260, 300);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);

    editor_ = new QTextEdit(this);
    editor_->setPlaceholderText(QStringLiteral("\u5728\u8fd9\u91cc\u8bb0\u5f55\u4fbf\u7b7e...")); // 在这里记录便签...
    editor_->setStyleSheet(
        "QTextEdit { background: rgba(255, 249, 196, 235); color: #5D4037;"
        " border: none; font-size: 14px; }");
    layout->addWidget(editor_);

    load();
    connect(editor_, &QTextEdit::textChanged, this, &StickyNoteWidget::save);
}

void StickyNoteWidget::load() {
    QFile f(noteFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    editor_->setPlainText(obj.value("text").toString());
    int x = obj.value("x").toInt(-1);
    int y = obj.value("y").toInt(-1);
    if (x >= 0 && y >= 0) {
        move(x, y);
    } else {
        move(200, 200);
    }
}

void StickyNoteWidget::save() {
    QJsonObject obj;
    obj["text"] = editor_->toPlainText();
    obj["x"] = pos().x();
    obj["y"] = pos().y();
    QFile f(noteFilePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson());
    }
}

void StickyNoteWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && !editor_->geometry().contains(e->position().toPoint())) {
        dragPos_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
        dragging_ = true;
        e->accept();
    }
    QWidget::mousePressEvent(e);
}

void StickyNoteWidget::mouseMoveEvent(QMouseEvent* e) {
    if (dragging_ && (e->buttons() & Qt::LeftButton)) {
        move(e->globalPosition().toPoint() - dragPos_);
        e->accept();
    }
    QWidget::mouseMoveEvent(e);
}

void StickyNoteWidget::mouseReleaseEvent(QMouseEvent* e) {
    dragging_ = false;
    QWidget::mouseReleaseEvent(e);
}

void StickyNoteWidget::closeEvent(QCloseEvent* e) {
    save();
    QWidget::closeEvent(e);
}

} // namespace mcclock::gui
