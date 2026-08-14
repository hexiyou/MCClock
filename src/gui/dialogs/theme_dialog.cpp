#include "theme_dialog.h"
#include "../theme_manager.h"
#include "../widgets/frameless_helper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QMouseEvent>

namespace mcclock::gui {

ThemeDialog::ThemeDialog(QWidget* parent)
    : QDialog(parent)
    , selectedColor_(ThemeManager::currentPrimaryColor())
{
    setWindowTitle(QStringLiteral("换肤"));
    setFixedSize(360, 220);

    // Build UI first, THEN apply frameless style
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    // Create color grid (2 rows x 3 columns)
    auto* grid = new QGridLayout();
    grid->setSpacing(12);

    QVector<QColor> colors = availableColors();
    int index = 0;
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (index >= colors.size()) break;

            QColor color = colors[index];
            auto* colorBtn = new QPushButton();
            colorBtn->setFixedSize(90, 50);
            colorBtn->setStyleSheet(QString(
                "QPushButton {"
                "  background-color: %1;"
                "  border: 3px solid transparent;"
                "  border-radius: 6px;"
                "}"
                "QPushButton:hover {"
                "  border: 3px solid #FFFFFF;"
                "  background-color: %2;"
                "}"
            ).arg(color.name(), color.lighter(120).name()));

            // Store color data
            colorBtn->setProperty("color", color);

            connect(colorBtn, &QPushButton::clicked, this, [this, color, colorBtn]() {
                selectedColor_ = color;
                emit colorSelected(color);
                accept();
            });

            grid->addWidget(colorBtn, row, col);
            ++index;
        }
    }

    layout->addLayout(grid);

    // Apply frameless style AFTER all controls are created
    FramelessHelper::applyToInlineDialog(this, QStringLiteral("\u6362\u80a4"));
}

void ThemeDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    FramelessHelper::showOverlay(parentWidget());
}

void ThemeDialog::closeEvent(QCloseEvent* event) {
    FramelessHelper::hideOverlay(parentWidget());
    QDialog::closeEvent(event);
}

void ThemeDialog::reject() {
    FramelessHelper::hideOverlay(parentWidget());
    QDialog::reject();
}

void ThemeDialog::accept() {
    FramelessHelper::hideOverlay(parentWidget());
    QDialog::accept();
}

QVector<QColor> ThemeDialog::availableColors() {
    return {
        QColor(35, 139, 227),    // 蓝色
        QColor(60, 64, 77),      // 深灰色
        QColor(94, 188, 66),     // 绿色
        QColor(203, 0, 100),     // 粉红色
        QColor(137, 87, 161),    // 紫色
        QColor(238, 148, 7)      // 橙色
    };
}

void ThemeDialog::mousePressEvent(QMouseEvent* event) {
    if (!framelessMousePress(this, event))
        QDialog::mousePressEvent(event);
}

void ThemeDialog::mouseMoveEvent(QMouseEvent* event) {
    if (!framelessMouseMove(this, event))
        QDialog::mouseMoveEvent(event);
}

void ThemeDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (!framelessMouseRelease(this, event))
        QDialog::mouseReleaseEvent(event);
}

} // namespace mcclock::gui
