#include "theme_manager.h"

#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPainterPath>

namespace mcclock::gui {

void ThemeManager::applyTheme(QApplication& app) {
    QFile file(":/styles/flat_theme.qss");
    if (file.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(file.readAll()));
        file.close();
    }
}

QIcon ThemeManager::appIcon() {
    QPixmap pix(64, 64);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Rounded square background
    QPainterPath bg;
    bg.addRoundedRect(QRectF(2, 2, 60, 60), 14, 14);
    p.fillPath(bg, QColor("#1E88E5"));

    // Clock face
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(32, 32), 20, 20);

    // Clock hands
    p.setPen(QPen(QColor("#1565C0"), 3, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(32, 32), QPointF(32, 20));
    p.drawLine(QPointF(32, 32), QPointF(42, 36));

    // Bell top
    p.setPen(QPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(20, 12), QPointF(26, 17));
    p.drawLine(QPointF(44, 12), QPointF(38, 17));

    p.end();
    return QIcon(pix);
}

QPixmap ThemeManager::circlePixmap(const QString& colorHex, int size) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(colorHex));
    p.drawEllipse(0, 0, size, size);
    p.end();
    return pix;
}

} // namespace mcclock::gui
