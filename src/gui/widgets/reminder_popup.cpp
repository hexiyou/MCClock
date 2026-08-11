#include "reminder_popup.h"
#include "core/dal/settings_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include <QPainter>

namespace mcclock::gui {

ReminderPopup::ReminderPopup(const QString& title, const QString& message, QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    setFixedSize(360, 180);
    setupUi(title, message);
}

void ReminderPopup::paintEvent(QPaintEvent*) {
    // QSS backgrounds are not painted on translucent custom widgets,
    // so draw the rounded blue card manually
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 136, 229)); // #1E88E5
    p.drawRoundedRect(rect(), 8, 8);
}

void ReminderPopup::setupUi(const QString& title, const QString& message) {
    setStyleSheet("ReminderPopup QLabel { color: white; background: transparent; }");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(8);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    root->addWidget(titleLabel);

    auto* msgLabel = new QLabel(message, this);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet("font-size: 13px;");
    root->addWidget(msgLabel, 1);

    auto* btnBar = new QHBoxLayout();
    btnBar->addStretch();
    auto* dismissBtn = new QPushButton(QStringLiteral("\u6211\u77e5\u9053\u4e86"), this); // 我知道了
    dismissBtn->setStyleSheet(
        "QPushButton { background: white; color: #1E88E5; border-radius: 4px; padding: 6px 16px; }"
        "QPushButton:hover { background: #E3F2FD; }");
    btnBar->addWidget(dismissBtn);
    root->addLayout(btnBar);

    connect(dismissBtn, &QPushButton::clicked, this, [this]() {
        emit dismissClicked();
        close();
    });

    // Auto close if configured
    auto& s = mcclock::dal::SettingsManager::instance();
    if (s.closeMode() == "auto") {
        QTimer::singleShot(s.autoCloseMinutes() * 60 * 1000, this, [this]() {
            if (isVisible()) close();
        });
    }
}

void ReminderPopup::showAtConfiguredPosition() {
    auto& s = mcclock::dal::SettingsManager::instance();
    QString pos = s.reminderPosition();
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) { show(); return; }
    QRect avail = screen->availableGeometry();

    if (pos == "left_up") {
        move(avail.left() + 20, avail.top() + 20);
    } else if (pos == "left_down") {
        move(avail.left() + 20, avail.bottom() - height() - 20);
    } else { // center
        move(avail.center().x() - width() / 2, avail.center().y() - height() / 2);
    }
    show();
    raise();
    activateWindow();
}

} // namespace mcclock::gui
