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

signals:
    void desktopClockToggled(bool visible);

private:
    void openCalendar();
    void openStickyNote();
    void openTimeCalculator();
    void openSystemCalculator();

    QPushButton* clockToggleBtn_ = nullptr;
    StickyNoteWidget* note_ = nullptr;
};

} // namespace mcclock::gui
