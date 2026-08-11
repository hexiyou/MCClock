#include "navigation_bar.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>

namespace mcclock::gui {

NavigationBar::NavigationBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("NavigationBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(52);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(2);

    auto* group = new QButtonGroup(this);
    group->setExclusive(true);

    const QStringList names = {
        QStringLiteral("\u9996\u9875"),           // 首页
        QStringLiteral("\u95f9\u949f\u63d0\u9192"), // 闹钟提醒
        QStringLiteral("\u751f\u65e5\u63d0\u9192"), // 生日提醒
        QStringLiteral("\u5b9a\u65f6\u5173\u673a"), // 定时关机
        QStringLiteral("\u8fd0\u884c\u7a0b\u5e8f"), // 运行程序
        QStringLiteral("\u5012\u8ba1\u65f6"),       // 倒计时
        QStringLiteral("\u8ba1\u65f6\u5668"),       // 计时器
        QStringLiteral("\u5065\u5eb7\u63d0\u9192")  // 健康提醒
    };

    for (int i = 0; i < names.size(); ++i) {
        auto* btn = new QPushButton(names[i], this);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        group->addButton(btn, i);
        layout->addWidget(btn);
        tabs_.append(btn);
    }

    connect(group, &QButtonGroup::idClicked, this, [this](int id) {
        emit currentIndexChanged(id);
    });

    layout->addStretch();

    // Skin switch button (right side)
    auto* skinBtn = new QPushButton(QStringLiteral("\u2602 \u76ae\u80a4"), this); // ☂ 皮肤
    skinBtn->setCursor(Qt::PointingHandCursor);
    connect(skinBtn, &QPushButton::clicked, this, &NavigationBar::skinClicked);
    layout->addWidget(skinBtn);

    // Settings button (right side)
    auto* settingsBtn = new QPushButton(QStringLiteral("\u2699 \u8bbe\u7f6e"), this); // ⚙ 设置
    settingsBtn->setCursor(Qt::PointingHandCursor);
    connect(settingsBtn, &QPushButton::clicked, this, &NavigationBar::settingsClicked);
    layout->addWidget(settingsBtn);

    if (!tabs_.isEmpty()) {
        tabs_.first()->setChecked(true);
    }
}

void NavigationBar::setCurrentIndex(int index) {
    if (index >= 0 && index < tabs_.size()) {
        tabs_[index]->setChecked(true);
        emit currentIndexChanged(index);
    }
}

int NavigationBar::currentIndex() const {
    for (int i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i]->isChecked()) return i;
    }
    return 0;
}

} // namespace mcclock::gui
