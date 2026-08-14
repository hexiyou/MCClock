#include "frameless_messagebox.h"
#include "frameless_helper.h"
#include "../theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDialog>
#include <QFormLayout>
#include <QStyle>
#include <QGraphicsDropShadowEffect>

namespace mcclock::gui {

// Helper to generate dynamic stylesheet based on current theme color
static QString generateDialogStyleSheet() {
    QColor accent = ThemeManager::currentPrimaryColor();
    QString accentHex = accent.name();
    
    // Calculate darker color for hover
    QColor darker = accent.darker(120);
    QString darkerHex = darker.name();
    
    return QStringLiteral(
        "QDialog { background: white; border: 2px solid %1; }"
        "QDialog QLabel { color: #333; font-size: 13px; padding: 10px; }"
        "QDialog QPushButton {"
        "  min-width: 80px; padding: 6px 16px; border-radius: 4px;"
        "  font-size: 13px; background: %1; color: white; border: none;"
        "}"
        "QDialog QPushButton:hover { background: %2; }"
    ).arg(accentHex, darkerHex);
}

// Helper to add shadow effect to a dialog
static void addShadowEffect(QDialog* dialog) {
    if (!dialog) return;
    auto* shadowEffect = new QGraphicsDropShadowEffect(dialog);
    shadowEffect->setBlurRadius(20);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(2);
    shadowEffect->setColor(QColor(0, 0, 0, 80));
    dialog->setGraphicsEffect(shadowEffect);
}

// ─── FramelessMessageBox ─────────────────────────────────────────────────

QMessageBox::StandardButton FramelessMessageBox::information(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    QMessageBox msgBox(QMessageBox::Information, title, text, buttons, parent);
    applyFramelessStyle(&msgBox);
    msgBox.setDefaultButton(defaultButton);
    int result = msgBox.exec();
    FramelessHelper::hideOverlay(parent);
    return static_cast<QMessageBox::StandardButton>(result);
}

QMessageBox::StandardButton FramelessMessageBox::warning(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    QMessageBox msgBox(QMessageBox::Warning, title, text, buttons, parent);
    applyFramelessStyle(&msgBox);
    msgBox.setDefaultButton(defaultButton);
    int result = msgBox.exec();
    FramelessHelper::hideOverlay(parent);
    return static_cast<QMessageBox::StandardButton>(result);
}

QMessageBox::StandardButton FramelessMessageBox::question(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    QMessageBox msgBox(QMessageBox::Question, title, text, buttons, parent);
    applyFramelessStyle(&msgBox);
    msgBox.setDefaultButton(defaultButton);
    int result = msgBox.exec();
    FramelessHelper::hideOverlay(parent);
    return static_cast<QMessageBox::StandardButton>(result);
}

QMessageBox::StandardButton FramelessMessageBox::critical(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    QMessageBox msgBox(QMessageBox::Critical, title, text, buttons, parent);
    applyFramelessStyle(&msgBox);
    msgBox.setDefaultButton(defaultButton);
    int result = msgBox.exec();
    FramelessHelper::hideOverlay(parent);
    return static_cast<QMessageBox::StandardButton>(result);
}

void FramelessMessageBox::applyFramelessStyle(QMessageBox* msgBox) {
    if (!msgBox) return;
    
    // Apply frameless window hint to remove native title bar
    msgBox->setWindowFlags(msgBox->windowFlags() | Qt::FramelessWindowHint);
    msgBox->setAttribute(Qt::WA_TranslucentBackground, false);
    
    // Apply dynamic stylesheet based on current theme color
    msgBox->setStyleSheet(generateDialogStyleSheet());
    
    // Add shadow effect
    addShadowEffect(msgBox);
}

// ─── FramelessInputDialog ────────────────────────────────────────────────

QString FramelessInputDialog::getText(
    QWidget* parent,
    const QString& title,
    const QString& label,
    QLineEdit::EchoMode mode,
    const QString& text,
    bool* ok,
    Qt::WindowFlags flags)
{
    QInputDialog dialog(parent, flags);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setTextValue(text);
    dialog.setTextEchoMode(mode);
    
    // Apply frameless window hint
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground, false);
    
    // Apply dynamic stylesheet
    dialog.setStyleSheet(generateDialogStyleSheet());
    
    // Add shadow effect
    addShadowEffect(&dialog);
    
    // Show overlay on parent
    if (parent) {
        FramelessHelper::showOverlay(parent);
    }
    
    // Execute dialog
    int result = dialog.exec();
    
    // Hide overlay
    FramelessHelper::hideOverlay(parent);
    
    if (ok) *ok = (result == QDialog::Accepted);
    return dialog.textValue();
}

int FramelessInputDialog::getInt(
    QWidget* parent,
    const QString& title,
    const QString& label,
    int value,
    int minValue,
    int maxValue,
    int step,
    bool* ok,
    Qt::WindowFlags flags)
{
    QInputDialog dialog(parent, flags);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setIntValue(value);
    dialog.setIntRange(minValue, maxValue);
    dialog.setIntStep(step);
    
    // Apply frameless window hint
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground, false);
    
    // Apply dynamic stylesheet
    dialog.setStyleSheet(generateDialogStyleSheet());
    
    // Add shadow effect
    addShadowEffect(&dialog);
    
    // Show overlay on parent
    if (parent) {
        FramelessHelper::showOverlay(parent);
    }
    
    // Execute dialog
    int result = dialog.exec();
    
    // Hide overlay
    FramelessHelper::hideOverlay(parent);
    
    if (ok) *ok = (result == QDialog::Accepted);
    return dialog.intValue();
}

double FramelessInputDialog::getDouble(
    QWidget* parent,
    const QString& title,
    const QString& label,
    double value,
    double minValue,
    double maxValue,
    int decimals,
    bool* ok,
    Qt::WindowFlags flags)
{
    QInputDialog dialog(parent, flags);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setDoubleValue(value);
    dialog.setDoubleRange(minValue, maxValue);
    dialog.setDoubleDecimals(decimals);
    
    // Apply frameless window hint
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground, false);
    
    // Apply dynamic stylesheet
    dialog.setStyleSheet(generateDialogStyleSheet());
    
    // Add shadow effect
    addShadowEffect(&dialog);
    
    // Show overlay on parent
    if (parent) {
        FramelessHelper::showOverlay(parent);
    }
    
    // Execute dialog
    int result = dialog.exec();
    
    // Hide overlay
    FramelessHelper::hideOverlay(parent);
    
    if (ok) *ok = (result == QDialog::Accepted);
    return dialog.doubleValue();
}

} // namespace mcclock::gui
