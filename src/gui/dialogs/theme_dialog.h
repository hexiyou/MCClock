#pragma once

#include <QDialog>
#include <QVector>
#include <QColor>

class QMouseEvent;
class QShowEvent;
class QCloseEvent;

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

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

public slots:
    void reject() override;
    void accept() override;

private:
    QColor selectedColor_;
};

} // namespace mcclock::gui
