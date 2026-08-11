#pragma once

#include <QWidget>

class QPushButton;

namespace mcclock::gui {

class StickyNoteWidget;

// Right-side utility toolbar: calendar, sticky note, time calculator,
// system calculator (calc.exe) and desktop clock toggle.
class SidebarWidget : public QWidget {
    Q_OBJECT
public:
    explicit SidebarWidget(QWidget* parent = nullptr);

    // Sticky note state accessors (used by the tray menu)
    bool stickyNoteVisible() const;
    void setStickyNoteVisible(bool visible);

    // Sync the clock toggle button state (e.g. when toggled from elsewhere)
    void setClockToggleChecked(bool checked);

signals:
    void desktopClockToggled(bool visible);

private:
    void openCalendar();
    void toggleStickyNote();
    void openTimeCalculator();
    void openSystemCalculator();

    QPushButton* clockToggleBtn_ = nullptr;
    QPushButton* noteBtn_ = nullptr;
    StickyNoteWidget* note_ = nullptr;
};

} // namespace mcclock::gui
