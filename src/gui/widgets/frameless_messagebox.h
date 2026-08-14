#pragma once

#include <QMessageBox>
#include <QInputDialog>
#include <QString>
#include <QWidget>

namespace mcclock::gui {

// Frameless message box wrapper
class FramelessMessageBox {
public:
    // Show information message box with frameless style
    static QMessageBox::StandardButton information(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    
    // Show warning message box with frameless style
    static QMessageBox::StandardButton warning(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    
    // Show question message box with frameless style
    static QMessageBox::StandardButton question(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    
    // Show critical message box with frameless style
    static QMessageBox::StandardButton critical(
        QWidget* parent,
        const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    
private:
    // Helper to apply frameless style to QMessageBox
    static void applyFramelessStyle(QMessageBox* msgBox);
};

// Frameless input dialog wrapper
class FramelessInputDialog {
public:
    // Show text input dialog with frameless style
    static QString getText(
        QWidget* parent,
        const QString& title,
        const QString& label,
        QLineEdit::EchoMode mode = QLineEdit::Normal,
        const QString& text = QString(),
        bool* ok = nullptr,
        Qt::WindowFlags flags = Qt::WindowFlags());
    
    // Show int input dialog with frameless style
    static int getInt(
        QWidget* parent,
        const QString& title,
        const QString& label,
        int value = 0,
        int minValue = -2147483647,
        int maxValue = 2147483647,
        int step = 1,
        bool* ok = nullptr,
        Qt::WindowFlags flags = Qt::WindowFlags());
    
    // Show double input dialog with frameless style
    static double getDouble(
        QWidget* parent,
        const QString& title,
        const QString& label,
        double value = 0,
        double minValue = -2147483647,
        double maxValue = 2147483647,
        int decimals = 1,
        bool* ok = nullptr,
        Qt::WindowFlags flags = Qt::WindowFlags());
    
private:
    // Helper to apply frameless style to QInputDialog
    static void applyFramelessStyle(QInputDialog* dialog, QWidget* parent);
};

} // namespace mcclock::gui
