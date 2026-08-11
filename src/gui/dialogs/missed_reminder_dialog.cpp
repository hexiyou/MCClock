#include "missed_reminder_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

namespace mcclock::gui {

MissedReminderDialog::MissedReminderDialog(const QList<models::Alarm>& missed,
                                           QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("\u9057\u6f0f\u63d0\u9192")); // 遗漏提醒
    resize(420, 320);
    setupUi(missed);
}

void MissedReminderDialog::setupUi(const QList<models::Alarm>& missed) {
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(QStringLiteral("\u8f6f\u4ef6\u672a\u8fd0\u884c\u671f\u95f4\uff0c\u4ee5\u4e0b\u95f9\u949f\u5df2\u7ecf\u54cd\u8fc7\uff1a"), this); // 软件未运行期间，以下闹钟已经响过：
    layout->addWidget(title);

    auto* list = new QListWidget(this);
    for (const auto& a : missed) {
        QString text = a.time;
        if (!a.label.isEmpty()) {
            text += QStringLiteral("  ") + a.label;
        }
        list->addItem(text);
    }
    layout->addWidget(list, 1);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* okBtn = new QPushButton(QStringLiteral("\u77e5\u9053\u4e86"), this); // 知道了
    btnRow->addWidget(okBtn);
    layout->addLayout(btnRow);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
}

} // namespace mcclock::gui
