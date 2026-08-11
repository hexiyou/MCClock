#pragma once

#include <QString>
#include <QIcon>
#include <QPixmap>

class QApplication;

namespace mcclock::gui {

// Loads and applies the flat theme; generates theme icons procedurally
// (no external asset files required).
class ThemeManager {
public:
    static void applyTheme(QApplication& app);

    // App / tray icon: a flat blue rounded square with a white clock face
    static QIcon appIcon();

    // Simple colored circle icon used for page placeholders
    static QPixmap circlePixmap(const QString& colorHex, int size = 16);
};

} // namespace mcclock::gui
