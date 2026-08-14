#include "frameless_helper.h"

#include <QMainWindow>
#include <QDialog>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>

namespace mcclock::gui {

// ─── Private data stored via QObject::setProperty ─────────────────────────

static constexpr const char* kHelperProp = "_framelessHelper";
static constexpr const char* kOverlayProp = "_framelessOverlay";

struct FramelessData {
    QWidget* titleBar = nullptr;
    QLabel* titleLabel = nullptr;
    QPushButton* minBtn = nullptr;
    QPushButton* maxBtn = nullptr;
    QPushButton* closeBtn = nullptr;
    bool maximizeEnabled = true;
    bool isMaximized = false;
    QRect normalGeometry;
    // Drag state
    bool dragging = false;
    QPoint dragPos;
    // Resize state
    bool resizing = false;
    Qt::Edges resizeEdges;
    QPoint resizePressGlobal;
    QSize resizeOrigSize;
    QRect resizeOrigGeo;
};

FramelessData* helperData(QWidget* w) {
    if (!w) return nullptr;
    QVariant v = w->property(kHelperProp);
    if (!v.isValid()) return nullptr;
    return static_cast<FramelessData*>(v.value<void*>());
}

// ─── Event filter for inline dialog drag ──────────────────────────────────

FramelessDragFilter::FramelessDragFilter(QObject* parent)
    : QObject(parent), dragging_(false) {}

bool FramelessDragFilter::eventFilter(QObject* obj, QEvent* event) {
    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (!widget) return false;
    
    QDialog* dialog = qobject_cast<QDialog*>(widget);
    if (!dialog) return false;
    
    // Check if this dialog has our frameless property
    QVariant v = dialog->property(kHelperProp);
    if (!v.isValid()) return false;
    
    FramelessData* d = static_cast<FramelessData*>(v.value<void*>());
    if (!d || !d->titleBar) return false;
    
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            QPoint localPos = me->position().toPoint();
            // Check if click is on title bar
            if (d->titleBar->geometry().contains(localPos)) {
                // Don't drag if click is on a button
                QWidget* child = widget->childAt(localPos);
                bool onButton = false;
                while (child) {
                    if (qobject_cast<QPushButton*>(child)) { onButton = true; break; }
                    child = child->parentWidget();
                }
                if (!onButton) {
                    dragging_ = true;
                    dragPos_ = me->globalPosition().toPoint() - widget->pos();
                    me->accept();
                    return true;
                }
            }
        }
        break;
    }
    case QEvent::MouseMove: {
        if (dragging_) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->buttons() & Qt::LeftButton) {
                QPoint newPos = me->globalPosition().toPoint() - dragPos_;
                widget->move(newPos);
                me->accept();
                return true;
            }
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        if (dragging_) {
            dragging_ = false;
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            me->accept();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

// ─── Overlay shadow functions ─────────────────────────────────────────────

void FramelessHelper::showOverlay(QWidget* parent) {
    if (!parent) return;
    
    // Check if overlay already exists
    QVariant v = parent->property(kOverlayProp);
    if (v.isValid()) return;
    
    // Create overlay widget
    auto* overlay = new QWidget(parent);
    overlay->setObjectName(QStringLiteral("framelessOverlay"));
    overlay->setStyleSheet(QStringLiteral(
        "#framelessOverlay { background-color: rgba(0, 0, 0, 128); }"
    ));
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    overlay->setCursor(Qt::WaitCursor);
    
    // Position overlay to cover the entire parent
    overlay->setGeometry(parent->rect());
    overlay->raise();
    overlay->show();
    
    // Store reference
    parent->setProperty(kOverlayProp, QVariant::fromValue(reinterpret_cast<void*>(overlay)));
}

void FramelessHelper::hideOverlay(QWidget* parent) {
    if (!parent) return;
    
    QVariant v = parent->property(kOverlayProp);
    if (!v.isValid()) return;
    
    QWidget* overlay = static_cast<QWidget*>(v.value<void*>());
    if (overlay) {
        overlay->deleteLater();
    }
    parent->setProperty(kOverlayProp, QVariant());
}

// ─── Helper: create a flat window button ──────────────────────────────────

static QPushButton* makeTitleBtn(const QString& text, bool isClose = false) {
    auto* btn = new QPushButton(text);
    btn->setObjectName(QStringLiteral("framelessBtn"));
    btn->setFixedSize(46, 32);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);

    QString base = QStringLiteral(
        "QPushButton {"
        "  border: none; background: transparent; color: #444;"
        "  font-size: 13px; font-family: 'Segoe UI', sans-serif;"
        "}"
        "QPushButton:hover { background: #E0E0E0; }"
    );
    if (isClose) {
        base = QStringLiteral(
            "QPushButton {"
            "  border: none; background: transparent; color: #444;"
            "  font-size: 13px; font-family: 'Segoe UI', sans-serif;"
            "}"
            "QPushButton:hover { background: #E81123; color: white; }"
        );
    }
    btn->setStyleSheet(base);
    return btn;
}

// ─── Edge detection for resize ────────────────────────────────────────────

static Qt::Edges edgeAt(const QPoint& pos, const QWidget* w, int margin = 6) {
    Qt::Edges edges;
    if (pos.x() <= margin) edges |= Qt::LeftEdge;
    else if (pos.x() >= w->width() - margin) edges |= Qt::RightEdge;
    if (pos.y() <= margin) edges |= Qt::TopEdge;
    else if (pos.y() >= w->height() - margin) edges |= Qt::BottomEdge;
    return edges;
}

static Qt::CursorShape cursorForEdges(Qt::Edges edges) {
    if (edges == (Qt::LeftEdge | Qt::TopEdge) || edges == (Qt::RightEdge | Qt::BottomEdge))
        return Qt::SizeFDiagCursor;
    if (edges == (Qt::RightEdge | Qt::TopEdge) || edges == (Qt::LeftEdge | Qt::BottomEdge))
        return Qt::SizeBDiagCursor;
    if (edges & (Qt::LeftEdge | Qt::RightEdge))
        return Qt::SizeHorCursor;
    if (edges & (Qt::TopEdge | Qt::BottomEdge))
        return Qt::SizeVerCursor;
    return Qt::ArrowCursor;
}

// ─── Mouse event handlers (called from MainWindow overrides) ──────────────

bool framelessMousePress(QWidget* w, QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) return false;
    auto* d = helperData(w);
    if (!d) return false;

    QPoint globalPos = e->globalPosition().toPoint();
    QPoint localPos = w->mapFromGlobal(globalPos);

    // Check if press is on title bar (for drag)
    if (d->titleBar && d->titleBar->geometry().contains(localPos)) {
        // Don't drag if click is on a button
        QWidget* child = w->childAt(localPos);
        bool onButton = false;
        while (child) {
            if (qobject_cast<QPushButton*>(child)) { onButton = true; break; }
            child = child->parentWidget();
        }
        if (!onButton) {
            if (w->isMaximized()) {
                // Restore from maximized before dragging
                w->showNormal();
                d->isMaximized = false;
                d->maxBtn->setText(QStringLiteral("\u25A1")); // □
                // Position window so mouse stays at same relative position
                int ratio = (globalPos.x() - d->normalGeometry.left()) * 100 / w->width();
                int newX = globalPos.x() - w->width() * ratio / 100;
                w->move(newX, 0);
                d->dragPos = e->globalPosition().toPoint() - w->pos();
                d->dragging = true;
                e->accept();
                return true;
            }
            d->dragPos = e->globalPosition().toPoint() - w->pos();
            d->dragging = true;
            e->accept();
            return true;
        }
    }

    // Check if press is on edge (for resize)
    Qt::Edges edges = edgeAt(localPos, w);
    if (edges != Qt::Edges()) {
        d->resizing = true;
        d->resizeEdges = edges;
        d->resizePressGlobal = globalPos;
        d->resizeOrigSize = w->size();
        d->resizeOrigGeo = w->geometry();
        e->accept();
        return true;
    }

    return false;
}

bool framelessMouseMove(QWidget* w, QMouseEvent* e) {
    auto* d = helperData(w);
    if (!d) return false;

    if (d->dragging && (e->buttons() & Qt::LeftButton)) {
        QPoint newPos = e->globalPosition().toPoint() - d->dragPos;
        if (auto* screen = QGuiApplication::screenAt(newPos)) {
            QRect sg = screen->availableGeometry();
            newPos.setX(qBound(sg.left(), newPos.x(), sg.right() - w->width()));
            newPos.setY(qBound(sg.top(), newPos.y(), sg.bottom() - w->height()));
        }
        w->move(newPos);
        e->accept();
        return true;
    }

    if (d->resizing && (e->buttons() & Qt::LeftButton)) {
        QPoint delta = e->globalPosition().toPoint() - d->resizePressGlobal;
        int newW = d->resizeOrigSize.width();
        int newH = d->resizeOrigSize.height();
        if (d->resizeEdges & Qt::LeftEdge) newW -= delta.x();
        if (d->resizeEdges & Qt::RightEdge) newW += delta.x();
        if (d->resizeEdges & Qt::TopEdge) newH -= delta.y();
        if (d->resizeEdges & Qt::BottomEdge) newH += delta.y();
        newW = qMax(newW, w->minimumWidth());
        newH = qMax(newH, w->minimumHeight());
        if (d->resizeEdges & Qt::LeftEdge) {
            QPoint tl = d->resizeOrigGeo.topLeft();
            tl.rx() += d->resizeOrigSize.width() - newW;
            w->move(tl);
        }
        if (d->resizeEdges & Qt::TopEdge) {
            QPoint tl = w->pos();
            tl.ry() += d->resizeOrigSize.height() - newH;
            w->move(tl);
        }
        w->resize(newW, newH);
        e->accept();
        return true;
    }

    // Update cursor for edges
    if (!d->dragging && !d->resizing) {
        Qt::Edges edges = edgeAt(w->mapFromGlobal(e->globalPosition().toPoint()), w);
        w->setCursor(cursorForEdges(edges));
    }
    return false;
}

bool framelessMouseRelease(QWidget* w, QMouseEvent* e) {
    Q_UNUSED(e);
    auto* d = helperData(w);
    if (!d) return false;
    if (d->dragging) { d->dragging = false; return true; }
    if (d->resizing) { d->resizing = false; w->setCursor(Qt::ArrowCursor); return true; }
    return false;
}

// ─── Setup ────────────────────────────────────────────────────────────────

QWidget* FramelessHelper::setup(QWidget* window, const QString& title) {
    auto* d = new FramelessData;
    window->setProperty(kHelperProp, QVariant::fromValue(reinterpret_cast<void*>(d)));

    window->setWindowFlags(window->windowFlags() | Qt::FramelessWindowHint);
    window->setAttribute(Qt::WA_TranslucentBackground, false);
    window->setMouseTracking(true);

    // Build title bar
    auto* titleBar = new QWidget(window);
    titleBar->setObjectName(QStringLiteral("framelessTitleBar"));
    titleBar->setFixedHeight(36);
    titleBar->setStyleSheet(QStringLiteral(
        "#framelessTitleBar { background: #F5F5F5; border-bottom: 1px solid #E0E0E0; }"
    ));

    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(12, 0, 4, 0);
    titleLayout->setSpacing(0);

    // App icon
    auto* iconLabel = new QLabel(titleBar);
    iconLabel->setPixmap(window->windowIcon().pixmap(18, 18));
    iconLabel->setFixedSize(22, 22);
    titleLayout->addWidget(iconLabel);
    titleLayout->addSpacing(8);

    // Title text
    auto* titleLabel = new QLabel(title.isEmpty() ? window->windowTitle() : title, titleBar);
    titleLabel->setStyleSheet(QStringLiteral(
        "color: #333; font-size: 13px; font-weight: 500; background: transparent;"
        "font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif;"
    ));
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    // Window buttons
    auto* minBtn = makeTitleBtn(QStringLiteral("\u2014"));     // —
    auto* maxBtn = makeTitleBtn(QStringLiteral("\u25A1"));     // □
    auto* closeBtn = makeTitleBtn(QStringLiteral("\u2715"), true); // ✕

    d->titleBar = titleBar;
    d->titleLabel = titleLabel;
    d->minBtn = minBtn;
    d->maxBtn = maxBtn;
    d->closeBtn = closeBtn;

    titleLayout->addWidget(minBtn);
    titleLayout->addWidget(maxBtn);
    titleLayout->addWidget(closeBtn);

    // Connect buttons
    QObject::connect(minBtn, &QPushButton::clicked, window, [window]() {
        window->showMinimized();
    });
    QObject::connect(maxBtn, &QPushButton::clicked, window, [window, d]() {
        if (!d->maximizeEnabled) return;
        if (window->isMaximized()) {
            window->showNormal();
            d->isMaximized = false;
            d->maxBtn->setText(QStringLiteral("\u25A1"));
        } else {
            d->normalGeometry = window->geometry();
            window->showMaximized();
            d->isMaximized = true;
            d->maxBtn->setText(QStringLiteral("\u29C9")); // ⧉ restore
        }
    });
    QObject::connect(closeBtn, &QPushButton::clicked, window, [window]() {
        window->close();
    });

    // Insert title bar at the top of the existing layout
    auto* mainWin = qobject_cast<QMainWindow*>(window);
    if (mainWin) {
        auto* existingCentral = mainWin->centralWidget();
        if (existingCentral) {
            auto* wrapper = new QWidget(window);
            auto* wrapperLayout = new QVBoxLayout(wrapper);
            wrapperLayout->setContentsMargins(0, 0, 0, 0);
            wrapperLayout->setSpacing(0);
            wrapperLayout->addWidget(titleBar);
            wrapperLayout->addWidget(existingCentral, 1);
            mainWin->setCentralWidget(wrapper);
        }
    }

    return titleBar;
}

void FramelessHelper::setMaximizeEnabled(QWidget* window, bool enabled) {
    auto* d = helperData(window);
    if (d) {
        d->maximizeEnabled = enabled;
        if (d->maxBtn) {
            d->maxBtn->setEnabled(enabled);
            d->maxBtn->setVisible(enabled);
        }
    }
}

QWidget* FramelessHelper::setupDialog(QDialog* dialog, const QString& title) {
    if (!dialog) return nullptr;
    
    auto* d = new FramelessData;
    dialog->setProperty(kHelperProp, QVariant::fromValue(reinterpret_cast<void*>(d)));
    
    dialog->setWindowFlags(dialog->windowFlags() | Qt::FramelessWindowHint);
    dialog->setAttribute(Qt::WA_TranslucentBackground, false);
    dialog->setMouseTracking(true);
    
    // Build title bar
    auto* titleBar = new QWidget(dialog);
    titleBar->setObjectName(QStringLiteral("framelessTitleBar"));
    titleBar->setFixedHeight(36);
    titleBar->setMinimumWidth(200);
    titleBar->setStyleSheet(QStringLiteral(
        "#framelessTitleBar { background: #F5F5F5; border-bottom: 1px solid #E0E0E0; }"
    ));
    
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(8, 0, 4, 0);
    titleLayout->setSpacing(0);
    
    // App icon
    auto* iconLabel = new QLabel(titleBar);
    iconLabel->setPixmap(dialog->windowIcon().pixmap(16, 16));
    iconLabel->setFixedSize(20, 20);
    titleLayout->addWidget(iconLabel);
    titleLayout->addSpacing(6);
    
    // Title text
    auto* titleLabel = new QLabel(title.isEmpty() ? dialog->windowTitle() : title, titleBar);
    titleLabel->setStyleSheet(QStringLiteral(
        "color: #333; font-size: 12px; font-weight: 500; background: transparent;"
        "font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif;"
    ));
    titleLayout->addWidget(titleLabel, 1);
    titleLayout->addStretch();
    
    // Window buttons (only close for dialogs) - use darker color for better visibility
    auto* closeBtn = makeTitleBtn(QStringLiteral("\u2715"), true); // ✕
    closeBtn->setFixedSize(46, 32);  // Use same size as makeTitleBtn default
    // Override color for better visibility on light background
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  border: none; background: transparent; color: #666;"
        "  font-size: 14px; font-family: 'Segoe UI', sans-serif;"
        "}"
        "QPushButton:hover { background: #E81123; color: white; }"
    ));
    
    d->titleBar = titleBar;
    d->titleLabel = titleLabel;
    d->closeBtn = closeBtn;
    d->maximizeEnabled = false;
    
    titleLayout->addWidget(closeBtn);
    
    // Connect close button
    QObject::connect(closeBtn, &QPushButton::clicked, dialog, [dialog]() {
        dialog->close();
    });
    
    // Get or create layout
    auto* existingLayout = dialog->layout();
    if (existingLayout) {
        // Save original margins and spacing
        QMargins originalMargins = existingLayout->contentsMargins();
        int originalSpacing = existingLayout->spacing();
        
        // Take all widgets from existing layout and re-parent them
        auto* contentWidget = new QWidget(dialog);
        auto* contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(originalMargins);  // Preserve original margins
        contentLayout->setSpacing(originalSpacing >= 0 ? originalSpacing : 6);  // Preserve original spacing or use default
        
        // Move items from old layout to new layout
        QLayoutItem* item;
        while ((item = existingLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                contentLayout->addWidget(item->widget());
            } else if (item->layout()) {
                contentLayout->addLayout(item->layout());
            } else {
                delete item;
            }
        }
        delete existingLayout;
        
        // Create wrapper with title bar on top
        auto* wrapper = new QWidget(dialog);
        auto* wrapperLayout = new QVBoxLayout(wrapper);
        wrapperLayout->setContentsMargins(0, 0, 0, 0);
        wrapperLayout->setSpacing(0);
        wrapperLayout->addWidget(titleBar);
        wrapperLayout->addWidget(contentWidget, 1);
        dialog->setLayout(wrapperLayout);
        
        // Ensure content widget expands properly
        contentWidget->setMinimumSize(0, 0);
        contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    } else {
        // No existing layout, create one with title bar and empty content area
        auto* wrapper = new QWidget(dialog);
        auto* wrapperLayout = new QVBoxLayout(wrapper);
        wrapperLayout->setContentsMargins(0, 0, 0, 0);
        wrapperLayout->setSpacing(0);
        wrapperLayout->addWidget(titleBar);
        // Add a stretch spacer for empty content area
        auto* contentArea = new QWidget(dialog);
        contentArea->setMinimumHeight(0);
        contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        wrapperLayout->addWidget(contentArea, 1);
        dialog->setLayout(wrapperLayout);
    }
    
    // Add shadow effect to dialog
    auto* shadowEffect = new QGraphicsDropShadowEffect(dialog);
    shadowEffect->setBlurRadius(20);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(2);
    shadowEffect->setColor(QColor(0, 0, 0, 80));
    dialog->setGraphicsEffect(shadowEffect);
    
    return titleBar;
}

void FramelessHelper::applyToInlineDialog(QDialog* dialog, const QString& title) {
    if (!dialog) return;
    
    // Skip if already applied
    if (dialog->property(kHelperProp).isValid()) return;
    
    auto* d = new FramelessData;
    dialog->setProperty(kHelperProp, QVariant::fromValue(reinterpret_cast<void*>(d)));
    
    dialog->setWindowFlags(dialog->windowFlags() | Qt::FramelessWindowHint);
    dialog->setAttribute(Qt::WA_TranslucentBackground, false);
    dialog->setMouseTracking(true);
    
    // Build title bar
    auto* titleBar = new QWidget(dialog);
    titleBar->setObjectName(QStringLiteral("framelessTitleBar"));
    titleBar->setFixedHeight(36);
    titleBar->setMinimumWidth(200);
    titleBar->setStyleSheet(QStringLiteral(
        "#framelessTitleBar { background: #F5F5F5; border-bottom: 1px solid #E0E0E0; }"
    ));
    
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(8, 0, 4, 0);
    titleLayout->setSpacing(0);
    
    // App icon
    auto* iconLabel = new QLabel(titleBar);
    iconLabel->setPixmap(dialog->windowIcon().pixmap(16, 16));
    iconLabel->setFixedSize(20, 20);
    titleLayout->addWidget(iconLabel);
    titleLayout->addSpacing(6);
    
    // Title text
    auto* titleLabel = new QLabel(title.isEmpty() ? dialog->windowTitle() : title, titleBar);
    titleLabel->setStyleSheet(QStringLiteral(
        "color: #333; font-size: 12px; font-weight: 500; background: transparent;"
        "font-family: 'Microsoft YaHei', 'Segoe UI', sans-serif;"
    ));
    titleLayout->addWidget(titleLabel, 1);
    titleLayout->addStretch();
    
    // Close button - use darker color for better visibility
    auto* closeBtn = makeTitleBtn(QStringLiteral("\u2715"), true);
    closeBtn->setFixedSize(46, 32);  // Use same size as makeTitleBtn default
    // Override color for better visibility on light background
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  border: none; background: transparent; color: #666;"
        "  font-size: 14px; font-family: 'Segoe UI', sans-serif;"
        "}"
        "QPushButton:hover { background: #E81123; color: white; }"
    ));
    
    d->titleBar = titleBar;
    d->titleLabel = titleLabel;
    d->closeBtn = closeBtn;
    d->maximizeEnabled = false;
    
    titleLayout->addWidget(closeBtn);
    
    QObject::connect(closeBtn, &QPushButton::clicked, dialog, [dialog]() {
        dialog->close();
    });
    
    // Get existing layout and wrap it
    auto* existingLayout = dialog->layout();
    if (existingLayout) {
        // Save original margins and spacing
        QMargins originalMargins = existingLayout->contentsMargins();
        int originalSpacing = existingLayout->spacing();
        
        // Take all items from existing layout
        auto* contentWidget = new QWidget(dialog);
        auto* contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(originalMargins);  // Preserve original margins
        contentLayout->setSpacing(originalSpacing >= 0 ? originalSpacing : 6);  // Preserve original spacing or use default
        
        QLayoutItem* item;
        while ((item = existingLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                contentLayout->addWidget(item->widget());
            } else if (item->layout()) {
                contentLayout->addLayout(item->layout());
            } else {
                delete item;
            }
        }
        delete existingLayout;
        
        // Create wrapper with title bar on top
        auto* wrapper = new QWidget(dialog);
        auto* wrapperLayout = new QVBoxLayout(wrapper);
        wrapperLayout->setContentsMargins(0, 0, 0, 0);
        wrapperLayout->setSpacing(0);
        wrapperLayout->addWidget(titleBar);
        wrapperLayout->addWidget(contentWidget, 1);
        dialog->setLayout(wrapperLayout);
        
        // Ensure content widget expands properly
        contentWidget->setMinimumSize(0, 0);
        contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    
    // Install event filter for drag functionality
    auto* filter = new FramelessDragFilter(dialog);
    dialog->installEventFilter(filter);
    
    // Add shadow effect to dialog
    auto* shadowEffect = new QGraphicsDropShadowEffect(dialog);
    shadowEffect->setBlurRadius(20);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(2);
    shadowEffect->setColor(QColor(0, 0, 0, 80));
    dialog->setGraphicsEffect(shadowEffect);
}

} // namespace mcclock::gui
