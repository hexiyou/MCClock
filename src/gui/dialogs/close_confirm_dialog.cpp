#include "close_confirm_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>

namespace mcclock::gui {

CloseConfirmDialog::CloseConfirmDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("\u5173\u95ed\u786e\u8ba4")); // 关闭确认
    setFixedSize(300, 190);
    setModal(true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(10);

    auto* title = new QLabel(QStringLiteral("\u70b9\u51fb\u5173\u95ed\u540e\uff0c\u60a8\u5e0c\u671b\uff1a"), this); // 点击关闭后，您希望：
    title->setProperty("hint", "title");
    root->addWidget(title);

    trayOption_ = new QRadioButton(
        QStringLiteral("\u6700\u5c0f\u5316\u5230\u7cfb\u7edf\u6258\u76d8\u533a\u4e0d\u9000\u51fa"), this); // 最小化到系统托盘区不退出
    trayOption_->setChecked(true);
    root->addWidget(trayOption_);

    exitOption_ = new QRadioButton(QStringLiteral("\u9000\u51fa\u7a0b\u5e8f"), this); // 退出程序
    root->addWidget(exitOption_);

    dontAskCheck_ = new QCheckBox(QStringLiteral("\u4e0d\u518d\u63d0\u793a"), this); // 不再提示
    root->addWidget(dontAskCheck_);

    root->addStretch();

    auto* btnBar = new QHBoxLayout();
    btnBar->addStretch();
    auto* okBtn = new QPushButton(QStringLiteral("\u786e\u5b9a"), this); // 确定
    auto* cancelBtn = new QPushButton(QStringLiteral("\u53d6\u6d88"), this); // 取消
    cancelBtn->setProperty("flatStyle", "secondary");
    btnBar->addWidget(okBtn);
    btnBar->addWidget(cancelBtn);
    root->addLayout(btnBar);

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

CloseConfirmDialog::Action CloseConfirmDialog::selectedAction() const {
    return exitOption_->isChecked() ? ExitProgram : MinimizeToTray;
}

bool CloseConfirmDialog::dontAskAgain() const {
    return dontAskCheck_->isChecked();
}

} // namespace mcclock::gui
