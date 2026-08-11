#pragma once

#include <QWidget>
#include <QList>

class QPushButton;

namespace mcclock::gui {

// Top navigation bar with 8 module tabs and settings/skin buttons.
// Flat blue background (styled via QSS #NavigationBar).
class NavigationBar : public QWidget {
    Q_OBJECT
public:
    explicit NavigationBar(QWidget* parent = nullptr);

    void setCurrentIndex(int index);
    int currentIndex() const;

signals:
    void currentIndexChanged(int index);
    void settingsClicked();
    void skinClicked();

private:
    QList<QPushButton*> tabs_;
};

} // namespace mcclock::gui
