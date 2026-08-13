#include "sticky_note_widget.h"
#include "core/utils/platform_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QGuiApplication>
#include <QFileDialog>
#include <QTextStream>
#include <QFont>
#include <QFontDialog>
#include <QSlider>
#include <functional>

namespace mcclock::gui {

namespace {

// QTextEdit subclass that intercepts Ctrl+Wheel for zoom
class ZoomableTextEdit : public QTextEdit {
public:
    using QTextEdit::QTextEdit;
    std::function<void(int)> onCtrlWheel;
protected:
    void wheelEvent(QWheelEvent* e) override {
        if (e->modifiers() & Qt::ControlModifier) {
            if (onCtrlWheel) onCtrlWheel(e->angleDelta().y());
            e->accept();
            return;
        }
        QTextEdit::wheelEvent(e);
    }
};

QString noteFilePath() {
    return mcclock::utils::PlatformUtils::appDataPath() + "/sticky_notes.json";
}

// Clamp window position so at least a small strip remains visible on screen
void clampPosToScreen(QWidget* w) {
    QRect screenGeo;
    if (auto* screen = QGuiApplication::primaryScreen())
        screenGeo = screen->availableGeometry();
    const int vis = 20;
    int nx = qBound(screenGeo.left() - w->width() + vis, w->pos().x(), screenGeo.right() - vis);
    int ny = qBound(screenGeo.top() - w->height() + vis, w->pos().y(), screenGeo.bottom() - vis);
    w->move(nx, ny);
}

} // namespace

StickyNoteWidget::StickyNoteWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setMinimumSize(150, 100);
    resize(260, 320);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 8);
    layout->setSpacing(4);

    // Title bar: drag handle area + buttons
    auto* header = new QHBoxLayout();
    header->setContentsMargins(4, 0, 0, 0);
    header->setSpacing(2);
    auto* title = new QLabel(QStringLiteral("\u4fbf\u7b7e"), this); // 便签
    title->setStyleSheet("color: #8D6E63; font-size: 12px; font-weight: bold; background: transparent;");
    header->addWidget(title);
    header->addStretch();

    // Save button
    saveBtn_ = new QPushButton(QString::fromUtf8("\xF0\x9F\x92\xBE"), this);
    saveBtn_->setFixedSize(24, 22);
    saveBtn_->setToolTip(QStringLiteral("\u4fdd\u5b58\u5230\u6587\u4ef6")); // 保存到文件
    saveBtn_->setCursor(Qt::PointingHandCursor);
    saveBtn_->setStyleSheet(makeBtnStyleSheet(bgColor_.name()));
    connect(saveBtn_, &QPushButton::clicked, this, &StickyNoteWidget::saveToFile);
    header->addWidget(saveBtn_);

    // Theme button
    themeBtn_ = new QPushButton(QString::fromUtf8("\xF0\x9F\x8E\xA8"), this);
    themeBtn_->setFixedSize(24, 22);
    themeBtn_->setToolTip(QStringLiteral("\u6362\u80a4\u80cc\u666f\u8272")); // 换肤背景色
    themeBtn_->setCursor(Qt::PointingHandCursor);
    themeBtn_->setStyleSheet(makeBtnStyleSheet(bgColor_.name()));
    connect(themeBtn_, &QPushButton::clicked, this, &StickyNoteWidget::showThemePopup);
    header->addWidget(themeBtn_);

    // Close button
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

    auto* ed = new ZoomableTextEdit(this);
    ed->setPlaceholderText(QStringLiteral("\u5728\u8fd9\u91cc\u8bb0\u5f55\u4fbf\u7b7e..."));
    ed->setStyleSheet(
        "QTextEdit { background: transparent; color: #5D4037;"
        " border: none; }");
    ed->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ed, &QWidget::customContextMenuRequested,
            this, &StickyNoteWidget::showEditorContextMenu);
    ed->onCtrlWheel = [this](int delta) {
        if (delta > 0 && fontSize_ < 40)
            fontSize_++;
        else if (delta < 0 && fontSize_ > 8)
            fontSize_--;
        updateEditorFont();
        save();
    };
    editor_ = ed;
    layout->addWidget(editor_);

    load();
    updateEditorFont();
    connect(editor_, &QTextEdit::textChanged, this, &StickyNoteWidget::save);
}

void StickyNoteWidget::load() {
    QFile f(noteFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    editor_->setPlainText(obj.value("text").toString());
    int w = obj.value("w").toInt(0);
    int h = obj.value("h").toInt(0);
    if (w >= minimumWidth() && h >= minimumHeight()) {
        resize(w, h);
    }
    int x = obj.value("x").toInt(-1);
    int y = obj.value("y").toInt(-1);
    if (x >= -100 && y >= -100) {
        move(x, y);
        clampPosToScreen(this);
    } else {
        move(200, 200);
    }
    // Load background color
    if (obj.contains("bg")) {
        bgColor_ = QColor(obj.value("bg").toString());
        if (!bgColor_.isValid()) bgColor_ = QColor(255, 249, 196, 240);
    }
    // Load opacity
    if (obj.contains("opacity")) {
        opacity_ = obj.value("opacity").toInt(240);
        opacity_ = qBound(60, opacity_, 255);
    }
    bgColor_.setAlpha(opacity_);
    // Load font size
    if (obj.contains("fontSize")) {
        fontSize_ = obj.value("fontSize").toInt(14);
        fontSize_ = qBound(8, fontSize_, 40);
    }
    // Load font family
    if (obj.contains("fontFamily")) {
        fontFamily_ = obj.value("fontFamily").toString();
    }
}

void StickyNoteWidget::save() {
    QJsonObject obj;
    obj["text"] = editor_->toPlainText();
    obj["x"] = pos().x();
    obj["y"] = pos().y();
    obj["w"] = width();
    obj["h"] = height();
    obj["bg"] = bgColor_.name(QColor::HexArgb);
    obj["fontSize"] = fontSize_;
    obj["opacity"] = opacity_;
    obj["fontFamily"] = fontFamily_;
    QFile f(noteFilePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson());
    }
}

void StickyNoteWidget::updateEditorFont() {
    QString family = fontFamily_.isEmpty() ? QStringLiteral("Microsoft YaHei") : fontFamily_;
    editor_->setStyleSheet(
        QString("QTextEdit { background: transparent; color: #5D4037;"
                " border: none; font-size: %1pt; font-family: \"%2\"; }")
        .arg(fontSize_).arg(family));
}

void StickyNoteWidget::applyOpacity(int alpha) {
    opacity_ = alpha;
    bgColor_.setAlpha(alpha);
    update();
    save();
}

void StickyNoteWidget::paintEvent(QPaintEvent* e) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(bgColor_);
    p.drawRoundedRect(rect(), 8, 8);
    QWidget::paintEvent(e);
}

bool StickyNoteWidget::eventFilter(QObject* obj, QEvent* e) {
    if (obj == closeBtn_) {
        switch (e->type()) {
        case QEvent::MouseButtonPress:
            return true;
        case QEvent::MouseButtonRelease:
            hide();
            return true;
        case QEvent::Enter:
            closeBtn_->setStyleSheet("QLabel { background: rgba(229, 57, 53, 0.85);"
                                     " color: #FFFFFF; border-radius: 4px; font-size: 15px; font-weight: bold; }");
            return true;
        case QEvent::Leave:
            closeBtn_->setStyleSheet(QString("QLabel { background: %1; color: #8D6E63;"
                                     " border-radius: 4px; font-size: 15px; font-weight: bold; }")
                                     .arg(bgColor_.name()));
            return true;
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, e);
}

void StickyNoteWidget::showEditorContextMenu(const QPoint& pos) {
    QMenu menu(this);
    bool hasSelection = editor_->textCursor().hasSelection();
    auto* undoAction = menu.addAction(QStringLiteral("\u64a4\u9500 (Ctrl+Z)"));     // 撤销
    auto* redoAction = menu.addAction(QStringLiteral("\u91cd\u505a (Ctrl+Y)"));     // 重做
    menu.addSeparator();
    auto* cutAction = menu.addAction(QStringLiteral("\u526a\u5207 (Ctrl+X)"));      // 剪切
    cutAction->setEnabled(hasSelection);
    auto* copyAction = menu.addAction(QStringLiteral("\u590d\u5236 (Ctrl+C)"));     // 复制
    copyAction->setEnabled(hasSelection);
    auto* pasteAction = menu.addAction(QStringLiteral("\u7c98\u8d34 (Ctrl+V)"));    // 粘贴
    auto* deleteAction = menu.addAction(QStringLiteral("\u5220\u9664 (Del)"));       // 删除
    deleteAction->setEnabled(hasSelection);
    menu.addSeparator();
    auto* selectAllAction = menu.addAction(QStringLiteral("\u5168\u9009 (Ctrl+A)")); // \u5168\u9009
    menu.addSeparator();
    auto* fontAction = menu.addAction(QStringLiteral("\u5b57\u4f53")); // \u5b57\u4f53
    QAction* triggered = menu.exec(editor_->mapToGlobal(pos));
    if (!triggered) return;
    if (triggered == undoAction) editor_->undo();
    else if (triggered == redoAction) editor_->redo();
    else if (triggered == cutAction) editor_->cut();
    else if (triggered == copyAction) editor_->copy();
    else if (triggered == pasteAction) editor_->paste();
    else if (triggered == deleteAction) editor_->textCursor().removeSelectedText();
    else if (triggered == selectAllAction) editor_->selectAll();
    else if (triggered == fontAction) {
        bool ok;
        QFont f = QFontDialog::getFont(&ok, editor_->font(), this);
        if (ok) {
            fontFamily_ = f.family();
            updateEditorFont();
            save();
        }
    }
}

void StickyNoteWidget::saveToFile() {
    QString path = QFileDialog::getSaveFileName(this,
        QStringLiteral("\u4fdd\u5b58\u4fbf\u7b7e\u5185\u5bb9"), // 保存便签内容
        QDir::homePath() + "/note.txt",
        QStringLiteral("Text Files (*.txt);;All Files (*)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << editor_->toPlainText();
}

void StickyNoteWidget::showThemePopup() {
    // Preset color palette (RGB only, alpha comes from opacity_)
    struct ColorEntry { int r, g, b; QString name; };
    const ColorEntry palette[] = {
        { 255, 249, 196, QStringLiteral("\u9ec4\u8272") },  // \u9ec4\u8272
        { 187, 236, 255, QStringLiteral("\u84dd\u8272") },  // \u84dd\u8272
        { 198, 255, 204, QStringLiteral("\u7eff\u8272") },  // \u7eff\u8272
        { 255, 205, 214, QStringLiteral("\u7c89\u8272") },  // \u7c89\u8272
        { 225, 200, 255, QStringLiteral("\u7d2b\u8272") },  // \u7d2b\u8272
        { 255, 224, 196, QStringLiteral("\u6a59\u8272") },  // \u6a59\u8272
    };

    auto* popup = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->setStyleSheet("QWidget { background: #FFFFFF; border: 1px solid #CFD8DC; border-radius: 4px; }");
    auto* vbox = new QVBoxLayout(popup);
    vbox->setContentsMargins(6, 6, 6, 6);
    vbox->setSpacing(6);

    // Color palette row
    auto* hbox = new QHBoxLayout();
    hbox->setSpacing(4);
    for (const auto& entry : palette) {
        QColor cellColor(entry.r, entry.g, entry.b, opacity_);
        auto* btn = new QPushButton(popup);
        btn->setFixedSize(28, 28);
        bool isCurrent = (bgColor_.red() == entry.r && bgColor_.green() == entry.g && bgColor_.blue() == entry.b);
        btn->setStyleSheet(QString(
            "QPushButton { background: %1; border: 2px solid %2; border-radius: 3px; }"
            "QPushButton:hover { border: 2px solid #1E88E5; }"
        ).arg(cellColor.name(), isCurrent ? "#1E88E5" : "#E0E0E0"));
        if (isCurrent) {
            btn->setText(QStringLiteral("\u2713"));
            btn->setStyleSheet(QString(
                "QPushButton { background: %1; border: 2px solid %2; border-radius: 3px;"
                " color: #333; font-weight: bold; font-size: 14px; }"
                "QPushButton:hover { border: 2px solid #1E88E5; }"
            ).arg(cellColor.name(), isCurrent ? "#1E88E5" : "#E0E0E0"));
        }
        connect(btn, &QPushButton::clicked, this, [this, entry]() {
            bgColor_ = QColor(entry.r, entry.g, entry.b, opacity_);
            applyThemeColor(bgColor_);
        });
        hbox->addWidget(btn);
    }
    vbox->addLayout(hbox);

    // Opacity slider
    auto* sliderRow = new QHBoxLayout();
    auto* sliderLabel = new QLabel(QStringLiteral("\u900f\u660e\u5ea6"), popup); // \u900f\u660e\u5ea6
    sliderLabel->setStyleSheet("color: #666; font-size: 11px; background: transparent;");
    sliderRow->addWidget(sliderLabel);
    auto* slider = new QSlider(Qt::Horizontal, popup);
    slider->setRange(60, 255);
    slider->setValue(opacity_);
    slider->setFixedWidth(120);
    slider->setStyleSheet("QSlider::groove:horizontal { height: 4px; background: #E0E0E0; border-radius: 2px; }"
                          "QSlider::handle:horizontal { background: #1E88E5; width: 14px; margin: -5px 0; border-radius: 7px; }");
    connect(slider, &QSlider::valueChanged, this, &StickyNoteWidget::applyOpacity);
    sliderRow->addWidget(slider);
    vbox->addLayout(sliderRow);

    popup->adjustSize();
    QPoint pos = themeBtn_->mapToGlobal(QPoint(0, themeBtn_->height() + 2));
    popup->move(pos);
    popup->show();
}

void StickyNoteWidget::applyThemeColor(const QColor& color) {
    bgColor_ = color;
    // Update close button background to match
    closeBtn_->setStyleSheet(QString(
        "QLabel { background: %1; color: #8D6E63;"
        " border-radius: 4px; font-size: 15px; font-weight: bold; }").arg(color.name()));
    // Update toolbar buttons
    QString btnStyle = makeBtnStyleSheet(color.name());
    saveBtn_->setStyleSheet(btnStyle);
    themeBtn_->setStyleSheet(btnStyle);
    update();
    save();
}

QString StickyNoteWidget::makeBtnStyleSheet(const QString& bg) {
    return QString(
        "QPushButton { padding: 0 2px; background-color: %1; color: #8D6E63;"
        " border: none; border-radius: 3px; font-size: 11px; font-weight: bold; }"
        "QPushButton:hover { background-color: %2; }"
    ).arg(bg, QColor(bg).lighter(115).name());
}

void StickyNoteWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        QPoint pos = e->position().toPoint();
        Qt::Edges edges = edgeAtPos(pos);
        if (edges != Qt::Edges()) {
            resizing_ = true;
            resizeEdges_ = edges;
            resizePressPos_ = e->globalPosition().toPoint();
            resizeOrigSize_ = size();
            e->accept();
            return;
        }
        if (!editor_->geometry().contains(pos)) {
            dragPos_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
            dragging_ = true;
            e->accept();
        }
    }
    QWidget::mousePressEvent(e);
}

void StickyNoteWidget::mouseMoveEvent(QMouseEvent* e) {
    QPoint pos = e->position().toPoint();
    if (resizing_ && (e->buttons() & Qt::LeftButton)) {
        QPoint delta = e->globalPosition().toPoint() - resizePressPos_;
        int newW = resizeOrigSize_.width();
        int newH = resizeOrigSize_.height();
        if (resizeEdges_ & Qt::LeftEdge) newW -= delta.x();
        if (resizeEdges_ & Qt::RightEdge) newW += delta.x();
        if (resizeEdges_ & Qt::TopEdge) newH -= delta.y();
        if (resizeEdges_ & Qt::BottomEdge) newH += delta.y();
        newW = qMax(newW, minimumWidth());
        newH = qMax(newH, minimumHeight());
        if (resizeEdges_ & Qt::LeftEdge) {
            QPoint topLeft = geometry().topLeft();
            topLeft.rx() += resizeOrigSize_.width() - newW;
            move(topLeft);
        }
        if (resizeEdges_ & Qt::TopEdge) {
            QPoint topLeft = geometry().topLeft();
            topLeft.ry() += resizeOrigSize_.height() - newH;
            move(topLeft);
        }
        resize(newW, newH);
        e->accept();
        return;
    }
    if (dragging_ && (e->buttons() & Qt::LeftButton)) {
        QPoint newPos = e->globalPosition().toPoint() - dragPos_;
        QRect screenGeo;
        if (auto* screen = QGuiApplication::primaryScreen())
            screenGeo = screen->availableGeometry();
        const int vis = 20;
        int nx = qBound(screenGeo.left() - width() + vis, newPos.x(), screenGeo.right() - vis);
        int ny = qBound(screenGeo.top() - height() + vis, newPos.y(), screenGeo.bottom() - vis);
        move(nx, ny);
        e->accept();
        return;
    }
    updateCursorForEdge(edgeAtPos(pos));
    QWidget::mouseMoveEvent(e);
}

void StickyNoteWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (resizing_) {
        resizing_ = false;
        resizeEdges_ = Qt::Edges();
        setCursor(Qt::ArrowCursor);
        save();
        e->accept();
        return;
    }
    if (dragging_) {
        dragging_ = false;
        save();
        e->accept();
        return;
    }
    QWidget::mouseReleaseEvent(e);
}

Qt::Edges StickyNoteWidget::edgeAtPos(const QPoint& pos) const {
    Qt::Edges edges;
    const int leftMargin = qMax(edgeMargin_, 10);
    const int topMargin = qMax(edgeMargin_, 10);
    if (pos.x() <= leftMargin) edges |= Qt::LeftEdge;
    else if (pos.x() >= width() - edgeMargin_) edges |= Qt::RightEdge;
    if (pos.y() <= topMargin) edges |= Qt::TopEdge;
    else if (pos.y() >= height() - edgeMargin_) edges |= Qt::BottomEdge;
    return edges;
}

void StickyNoteWidget::updateCursorForEdge(Qt::Edges edges) {
    if (edges == (Qt::LeftEdge | Qt::TopEdge) || edges == (Qt::RightEdge | Qt::BottomEdge))
        setCursor(Qt::SizeFDiagCursor);
    else if (edges == (Qt::RightEdge | Qt::TopEdge) || edges == (Qt::LeftEdge | Qt::BottomEdge))
        setCursor(Qt::SizeBDiagCursor);
    else if (edges & (Qt::LeftEdge | Qt::RightEdge))
        setCursor(Qt::SizeHorCursor);
    else if (edges & (Qt::TopEdge | Qt::BottomEdge))
        setCursor(Qt::SizeVerCursor);
    else
        setCursor(Qt::ArrowCursor);
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
