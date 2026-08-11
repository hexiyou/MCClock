#include "sticky_note_widget.h"
#include "core/utils/platform_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QPainter>
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
    setFixedSize(260, 320);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 8);
    layout->setSpacing(4);

    // Title bar: drag handle area + close button
    auto* header = new QHBoxLayout();
    header->setContentsMargins(4, 0, 0, 0);
    header->setSpacing(0);
    auto* title = new QLabel(QStringLiteral("\u4fbf\u7b7e"), this); // 便签
    title->setStyleSheet("color: #8D6E63; font-size: 12px; font-weight: bold; background: transparent;");
    header->addWidget(title);
    header->addStretch();
    auto* closeBtn = new QLabel(QStringLiteral("\u00d7"), this); // ×
    closeBtn_ = closeBtn;
    closeBtn->setFixedSize(22, 22);
    closeBtn->setAlignment(Qt::AlignCenter);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QLabel { background: rgba(255, 249, 196, 240); color: #8D6E63;"
                            " border-radius: 4px; font-size: 15px; font-weight: bold; }");
    closeBtn->installEventFilter(this);
    header->addWidget(closeBtn);
    layout->addLayout(header);

    editor_ = new QTextEdit(this);
    editor_->setPlaceholderText(QStringLiteral("\u5728\u8fd9\u91cc\u8bb0\u5f55\u4fbf\u7b7e...")); // 在这里记录便签...
    editor_->setStyleSheet(
        "QTextEdit { background: transparent; color: #5D4037;"
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

void StickyNoteWidget::paintEvent(QPaintEvent* e) {
    // Layered (translucent) top-level window: QSS backgrounds on custom
    // QWidget subclasses are unreliable here, so paint the note body manually.
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 249, 196, 240));
    p.drawRoundedRect(rect(), 8, 8);
    QWidget::paintEvent(e);
}

bool StickyNoteWidget::eventFilter(QObject* obj, QEvent* e) {
    if (obj == closeBtn_) {
        switch (e->type()) {
        case QEvent::MouseButtonPress:
            return true; // consume: do not start a window drag from the button
        case QEvent::MouseButtonRelease:
            hide();
            return true;
        case QEvent::Enter:
            closeBtn_->setStyleSheet("QLabel { background: rgba(229, 57, 53, 0.85);"
                                     " color: #FFFFFF; border-radius: 4px; font-size: 15px; font-weight: bold; }");
            return true;
        case QEvent::Leave:
            closeBtn_->setStyleSheet("QLabel { background: rgba(255, 249, 196, 240); color: #8D6E63;"
                                     " border-radius: 4px; font-size: 15px; font-weight: bold; }");
            return true;
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, e);
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
    if (dragging_) {
        dragging_ = false;
        save();
        e->accept();
        return;
    }
    QWidget::mouseReleaseEvent(e);
}

void StickyNoteWidget::closeEvent(QCloseEvent* e) {
    save();
    QWidget::closeEvent(e);
}

void StickyNoteWidget::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    emit visibilityChanged(true);
}

void StickyNoteWidget::hideEvent(QHideEvent* e) {
    save();
    QWidget::hideEvent(e);
    emit visibilityChanged(false);
}

} // namespace mcclock::gui
