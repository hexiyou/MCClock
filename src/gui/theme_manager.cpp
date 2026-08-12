#include "theme_manager.h"

#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPainterPath>

namespace mcclock::gui {

// Default primary color
QColor ThemeManager::s_primaryColor = QColor("#1E88E5");

void ThemeManager::applyTheme(QApplication& app) {
    QFile file(":/styles/flat_theme.qss");
    if (file.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(file.readAll()));
        file.close();
    }
}

void ThemeManager::applyTheme(QApplication& app, const QColor& primaryColor) {
    s_primaryColor = primaryColor;
    QString stylesheet = generateStyleSheet(primaryColor);
    app.setStyleSheet(stylesheet);
}

QColor ThemeManager::currentPrimaryColor() {
    return s_primaryColor;
}

void ThemeManager::setPrimaryColor(const QColor& color) {
    s_primaryColor = color;
}

QString ThemeManager::generateStyleSheet(const QColor& primaryColor) {
    // Calculate derived colors
    QColor dark = primaryColor.darker(130);
    QColor light = primaryColor.lighter(130);
    QColor hover = primaryColor.lighter(115);
    QColor pressed = primaryColor.darker(115);
    QColor selectedBg = primaryColor.lighter(140);  // Less bright for better contrast
    QColor selectedText = primaryColor.darker(150);
    QColor accentText = primaryColor;

    // Build stylesheet
    QString qss = QStringLiteral(R"(
/* ── MCClock Flat Theme (Dynamic) ── */
/* Primary: %1  Dark: %2  Light: %3 */

* {
    font-family: "Microsoft YaHei UI", "Segoe UI", sans-serif;
    font-size: 13px;
    outline: none;
}

QMainWindow, QDialog {
    background-color: #F5F7FA;
}

QWidget {
    color: #2B2F33;
}

/* ── Navigation bar ── */
#NavigationBar {
    background-color: %2;
}

#NavigationBar QPushButton {
    background: transparent;
    color: #FFFFFF;
    border: none;
    padding: 10px 6px;
    font-size: 12px;
}

#NavigationBar QPushButton:hover {
    color: #FFFFFF;
    background-color: rgba(255, 255, 255, 0.15);
}

#NavigationBar QPushButton:checked {
    color: #FFFFFF;
    background-color: %1;
    font-weight: bold;
}

/* ── Buttons ── */
QPushButton {
    background-color: %1;
    color: #FFFFFF;
    border: none;
    border-radius: 4px;
    padding: 7px 18px;
}

QPushButton:hover {
    background-color: %4;
}

QPushButton:pressed {
    background-color: %2;
}

QPushButton:disabled {
    background-color: #B0BEC5;
    color: #ECEFF1;
}

QPushButton[flatStyle="secondary"] {
    background-color: #ECEFF1;
    color: #37474F;
}

QPushButton[flatStyle="secondary"]:hover {
    background-color: #CFD8DC;
}

QPushButton[flatStyle="secondary"]:checked {
    background-color: %1;
    color: #FFFFFF;
}

QPushButton[flatStyle="danger"] {
    background-color: #E53935;
}

QPushButton[flatStyle="danger"]:hover {
    background-color: #EF5350;
}

/* ── Input widgets ── */
QLineEdit, QSpinBox, QDoubleSpinBox, QTimeEdit, QDateEdit, QDateTimeEdit, QComboBox, QTextEdit, QPlainTextEdit {
    background-color: #FFFFFF;
    border: 1px solid #CFD8DC;
    border-radius: 4px;
    padding: 5px 8px;
    selection-background-color: %1;
    selection-color: #FFFFFF;
}

QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QTimeEdit:focus, QDateEdit:focus, QTextEdit:focus {
    border: 1px solid %1;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: center right;
    border: none;
    width: 24px;
    image: url(:/icons/combo_down.png);
}

QComboBox QAbstractItemView {
    background-color: #FFFFFF;
    border: 1px solid #CFD8DC;
    selection-background-color: #E3F2FD;
    selection-color: #2B2F33;
    outline: none;
}

/* ── Spin box arrows ── */
QSpinBox::up-button, QDoubleSpinBox::up-button, QTimeEdit::up-button,
QDateEdit::up-button, QDateTimeEdit::up-button {
    subcontrol-origin: border;
    subcontrol-position: top right;
    width: 16px;
    border: none;
    image: url(:/icons/spin_up.png);
}

QSpinBox::down-button, QDoubleSpinBox::down-button, QTimeEdit::down-button,
QDateEdit::down-button, QDateTimeEdit::down-button {
    subcontrol-origin: border;
    subcontrol-position: bottom right;
    width: 16px;
    border: none;
    image: url(:/icons/spin_down.png);
}

QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover, QTimeEdit::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover, QTimeEdit::down-button:hover {
    background-color: %6;
}

/* ── Check boxes / radio buttons ── */
QCheckBox, QRadioButton {
    spacing: 6px;
}

QCheckBox::indicator, QRadioButton::indicator {
    width: 16px;
    height: 16px;
}

QCheckBox::indicator:unchecked {
    border: 1px solid #90A4AE;
    border-radius: 3px;
    background: #FFFFFF;
}

QCheckBox::indicator:checked {
    border: 1px solid %1;
    border-radius: 3px;
    background: %1;
    image: url(:/icons/check.png);
}

QRadioButton::indicator:unchecked {
    border: none;
    background: transparent;
    image: url(:/icons/radio_off.png);
}

QRadioButton::indicator:checked {
    border: none;
    background: transparent;
    image: url(:/icons/radio_on.png);
}

/* ── Tables ── */
QTableView, QTableWidget, QListView, QListWidget, QTreeView {
    background-color: #FFFFFF;
    border: 1px solid #E0E4E8;
    border-radius: 4px;
    gridline-color: #ECEFF1;
    alternate-background-color: #FAFBFC;
}

QHeaderView::section {
    background-color: #F0F3F6;
    color: #546E7A;
    border: none;
    border-bottom: 1px solid #E0E4E8;
    padding: 8px 10px;
    font-weight: bold;
}

QTableView::item:selected, QListView::item:selected {
    background-color: %6;
    color: %2;
}

/* ── Tabs ── */
QTabWidget::pane {
    border: 1px solid #E0E4E8;
    border-radius: 4px;
    background: #FFFFFF;
}

QTabBar::tab {
    background: #ECEFF1;
    color: #546E7A;
    padding: 8px 20px;
    border: none;
    border-top-left-radius: 4px;
    border-top-right-radius: 4px;
    margin-right: 2px;
}

QTabBar::tab:selected {
    background: #FFFFFF;
    color: %1;
    font-weight: bold;
}

/* ── Group boxes ── */
QGroupBox {
    border: 1px solid #E0E4E8;
    border-radius: 4px;
    margin-top: 12px;
    padding-top: 8px;
    background: #FFFFFF;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
    color: %1;
}

/* ── Scroll bars ── */
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background: #C5CDD3;
    border-radius: 5px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background: #90A4AE;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    background: transparent;
    height: 10px;
    margin: 0;
}

QScrollBar::handle:horizontal {
    background: #C5CDD3;
    border-radius: 5px;
    min-width: 30px;
}

/* ── Tooltips ── */
QToolTip {
    background-color: #37474F;
    color: #FFFFFF;
    border: none;
    padding: 5px 8px;
    border-radius: 3px;
}

/* ── Progress bar ── */
QProgressBar {
    background-color: #ECEFF1;
    border: none;
    border-radius: 4px;
    text-align: center;
    height: 10px;
}

QProgressBar::chunk {
    background-color: %1;
    border-radius: 4px;
}

/* ── Menu ── */
QMenu {
    background-color: #FFFFFF;
    border: 1px solid #E0E4E8;
    border-radius: 4px;
    padding: 4px;
}

QMenu::item {
    padding: 6px 24px;
    border-radius: 3px;
}

QMenu::item:selected {
    background-color: %6;
    color: %2;
}

/* ── Dialog buttons area ── */
#DialogButtonBar {
    background-color: #F5F7FA;
}

/* ── Status / hint labels ── */
QLabel[hint="title"] {
    font-size: 16px;
    font-weight: bold;
    color: #2B2F33;
}

QLabel[hint="subtitle"] {
    color: #78909C;
}

QLabel[hint="accent"] {
    color: %1;
}
)")
    .arg(primaryColor.name())         // %1 - primary
    .arg(dark.name())                 // %2 - dark
    .arg(light.name())                // %3 - light
    .arg(hover.name())                // %4 - hover
    .arg(pressed.name())              // %5 - pressed
    .arg(selectedBg.name())           // %6 - selected background
    .arg(selectedText.name())         // %7 - selected text
    .arg(accentText.name());          // %8 - accent text

    return qss;
}

QIcon ThemeManager::appIcon() {
    return appIcon(s_primaryColor);
}

QIcon ThemeManager::appIcon(const QColor& primaryColor) {
    // Prefer the bundled icon resource
    QIcon res(":/icons/app.png");
    if (!res.isNull()) return res;

    QPixmap pix(64, 64);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Rounded square background
    QColor dark = primaryColor.darker(130);
    QPainterPath bg;
    bg.addRoundedRect(QRectF(2, 2, 60, 60), 14, 14);
    p.fillPath(bg, primaryColor);

    // Clock face
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(32, 32), 20, 20);

    // Clock hands
    p.setPen(QPen(dark, 3, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(32, 32), QPointF(32, 20));
    p.drawLine(QPointF(32, 32), QPointF(42, 36));

    // Bell top
    p.setPen(QPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(20, 12), QPointF(26, 17));
    p.drawLine(QPointF(44, 12), QPointF(38, 17));

    p.end();
    return QIcon(pix);
}

QPixmap ThemeManager::circlePixmap(const QString& colorHex, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(colorHex));
    p.drawEllipse(0, 0, size, size);
    p.end();
    return pix;
}

} // namespace mcclock::gui
