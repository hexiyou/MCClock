#pragma once

#include <QDialog>
#include <QVector>
#include <QColor>

namespace mcclock::gui {

class ThemeDialog : public QDialog {
    Q_OBJECT
public:
    explicit ThemeDialog(QWidget* parent = nullptr);

    // Available theme colors
    static QVector<QColor> availableColors();

    // Get selected color
    QColor selectedColor() const { return selectedColor_; }

signals:
    void colorSelected(const QColor& color);

private:
    QColor selectedColor_;
};

} // namespace mcclock::gui
