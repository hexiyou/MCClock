#pragma once

#include <QString>
#include <QIcon>
#include <QPixmap>
#include <QColor>

class QApplication;

namespace mcclock::gui {

// Loads and applies the flat theme; generates theme icons procedurally
// (no external asset files required).
class ThemeManager {
public:
    static void applyTheme(QApplication& app);

    // Apply theme with custom primary color
    static void applyTheme(QApplication& app, const QColor& primaryColor);

    // Get current primary color
    static QColor currentPrimaryColor();

    // Set current primary color
    static void setPrimaryColor(const QColor& color);

    // App / tray icon: a flat blue rounded square with a white clock face
    static QIcon appIcon();

    // App / tray icon with custom color
    static QIcon appIcon(const QColor& primaryColor);

    // Simple colored circle icon used for page placeholders
    static QPixmap circlePixmap(const QString& colorHex, int size = 16);

private:
    static QColor s_primaryColor;
    static QString generateStyleSheet(const QColor& primaryColor);
};

} // namespace mcclock::gui
