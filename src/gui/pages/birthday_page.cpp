#include "birthday_page.h"
#include "core/services/business_services.h"
#include "core/services/lunar_calendar.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QTimeEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>

namespace mcclock::gui {

using mcclock::services::BirthdayService;
using mcclock::services::LunarCalendar;

namespace {

// Birthday add/edit dialog: Gregorian/Lunar switch + avatar + advance days
bool birthdayForm(QWidget* parent, mcclock::models::Birthday& b, bool isNew) {
    QDialog dlg(parent);
    dlg.setWindowTitle(isNew ? QStringLiteral("\u65b0\u589e\u751f\u65e5")   // 新增生日
                             : QStringLiteral("\u7f16\u8f91\u751f\u65e5")); // 编辑生日
    dlg.resize(420, 340);
    auto* layout = new QFormLayout(&dlg);

    auto* nameEdit = new QLineEdit(b.name, &dlg);
    layout->addRow(QStringLiteral("\u59d3\u540d\uff1a"), nameEdit); // 姓名：

    auto* genderCombo = new QComboBox(&dlg);
    genderCombo->addItem(QStringLiteral("\u7537"), 0); // 男
    genderCombo->addItem(QStringLiteral("\u5973"), 1); // 女
    genderCombo->setCurrentIndex(b.gender == 1 ? 1 : 0);
    layout->addRow(QStringLiteral("\u6027\u522b\uff1a"), genderCombo); // 性别：

    auto* solarRadio = new QRadioButton(QStringLiteral("\u516c\u5386"), &dlg); // 公历
    auto* lunarRadio = new QRadioButton(QStringLiteral("\u519c\u5386"), &dlg); // 农历
    if (b.isLunar) lunarRadio->setChecked(true); else solarRadio->setChecked(true);

    auto* dateEdit = new QDateEdit(&dlg);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setCalendarPopup(true);
    if (!isNew && !b.isLunar && b.solarYear > 0) {
        dateEdit->setDate(QDate(b.solarYear, b.solarMonth, b.solarDay));
    } else {
        dateEdit->setDate(QDate(1990, 1, 1));
    }

    auto* lunarMonthSpin = new QSpinBox(&dlg);
    lunarMonthSpin->setRange(1, 12);
    lunarMonthSpin->setPrefix(QStringLiteral("\u6708 ")); // 月
    auto* lunarDaySpin = new QSpinBox(&dlg);
    lunarDaySpin->setRange(1, 30);
    lunarDaySpin->setPrefix(QStringLiteral("\u65e5 ")); // 日
    if (!isNew && b.isLunar) {
        lunarMonthSpin->setValue(qBound(1, b.lunarMonth, 12));
        lunarDaySpin->setValue(qBound(1, b.lunarDay, 30));
    }
    auto* lunarRow = new QHBoxLayout();
    lunarRow->addWidget(lunarMonthSpin);
    lunarRow->addWidget(lunarDaySpin);

    auto* calendarRow = new QHBoxLayout();
    calendarRow->addWidget(solarRadio);
    calendarRow->addWidget(lunarRadio);
    calendarRow->addWidget(dateEdit);
    calendarRow->addWidget(lunarMonthSpin);
    calendarRow->addWidget(lunarDaySpin);
    calendarRow->addStretch();
    layout->addRow(QStringLiteral("\u751f\u65e5\uff1a"), calendarRow); // 生日：

    auto applyCalendar = [solarRadio, dateEdit, lunarMonthSpin, lunarDaySpin](bool lunar) {
        dateEdit->setVisible(!lunar);
        lunarMonthSpin->setVisible(lunar);
        lunarDaySpin->setVisible(lunar);
    };
    QObject::connect(lunarRadio, &QRadioButton::toggled, &dlg, applyCalendar);
    applyCalendar(b.isLunar);

    auto* timeEdit = new QTimeEdit(&dlg);
    timeEdit->setDisplayFormat("HH:mm");
    timeEdit->setTime(QTime::fromString(b.remindTime.isEmpty() ? "08:00" : b.remindTime, "HH:mm"));
    layout->addRow(QStringLiteral("\u63d0\u9192\u65f6\u95f4\uff1a"), timeEdit); // 提醒时间：

    auto* advanceSpin = new QSpinBox(&dlg);
    advanceSpin->setRange(1, 5);
    advanceSpin->setValue(qBound(1, b.advanceDays, 5));
    advanceSpin->setSuffix(QStringLiteral(" \u5929")); // 天
    layout->addRow(QStringLiteral("\u63d0\u524d\u63d0\u9192\uff1a"), advanceSpin); // 提前提醒：

    auto* avatarRow = new QHBoxLayout();
    auto* avatarEdit = new QLineEdit(b.avatarPath, &dlg);
    avatarEdit->setPlaceholderText(QStringLiteral("\u53ef\u9009\uff0c\u5934\u50cf\u56fe\u7247\u8def\u5f84")); // 可选，头像图片路径
    auto* browseBtn = new QPushButton(QStringLiteral("\u6d4f\u89c8..."), &dlg); // 浏览...
    avatarRow->addWidget(avatarEdit, 1);
    avatarRow->addWidget(browseBtn);
    layout->addRow(QStringLiteral("\u5934\u50cf\uff1a"), avatarRow); // 头像：
    QObject::connect(browseBtn, &QPushButton::clicked, &dlg, [avatarEdit, &dlg]() {
        QString f = QFileDialog::getOpenFileName(&dlg,
            QStringLiteral("\u9009\u62e9\u5934\u50cf"), QString(), // 选择头像
            QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
        if (!f.isEmpty()) avatarEdit->setText(f);
    });

    auto* labelEdit = new QLineEdit(b.label, &dlg);
    layout->addRow(QStringLiteral("\u5907\u6ce8\uff1a"), labelEdit); // 备注：

    auto* btnRow = new QHBoxLayout();
    auto* okBtn = new QPushButton(QStringLiteral("\u4fdd\u5b58"), &dlg);   // 保存
    auto* cancelBtn = new QPushButton(QStringLiteral("\u53d6\u6d88"), &dlg); // 取消
    cancelBtn->setProperty("flatStyle", "secondary");
    btnRow->addStretch();
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    layout->addRow(btnRow);
    QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;

    if (nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(&dlg, QStringLiteral("\u63d0\u793a"),
            QStringLiteral("\u8bf7\u8f93\u5165\u59d3\u540d")); // 请输入姓名
        return false;
    }
    b.name = nameEdit->text().trimmed();
    b.gender = genderCombo->currentData().toInt();
    b.isLunar = lunarRadio->isChecked();
    if (b.isLunar) {
        b.lunarMonth = lunarMonthSpin->value();
        b.lunarDay = lunarDaySpin->value();
    } else {
        QDate d = dateEdit->date();
        b.solarYear = d.year();
        b.solarMonth = d.month();
        b.solarDay = d.day();
    }
    b.remindTime = timeEdit->time().toString("HH:mm");
    b.advanceDays = advanceSpin->value();
    b.avatarPath = avatarEdit->text().trimmed();
    b.label = labelEdit->text();
    return true;
}

} // namespace

BirthdayPage::BirthdayPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refresh();
}

void BirthdayPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout();
    auto* addBtn = new QPushButton(QStringLiteral("\uff0b \u65b0\u589e\u751f\u65e5"), this); // ＋ 新增生日
    auto* editBtn = new QPushButton(QStringLiteral("\u7f16\u8f91"), this); // 编辑
    auto* delBtn = new QPushButton(QStringLiteral("\u5220\u9664"), this);  // 删除
    editBtn->setProperty("flatStyle", "secondary");
    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(delBtn);
    toolbar->addStretch();
    root->addLayout(toolbar);

    list_ = new QListWidget(this);
    list_->setSpacing(4);
    root->addWidget(list_, 1);

    auto* pagerRow = new QHBoxLayout();
    prevBtn_ = new QPushButton(QStringLiteral("\u4e0a\u4e00\u9875"), this); // 上一页
    nextBtn_ = new QPushButton(QStringLiteral("\u4e0b\u4e00\u9875"), this); // 下一页
    prevBtn_->setProperty("flatStyle", "secondary");
    nextBtn_->setProperty("flatStyle", "secondary");
    pageLabel_ = new QLabel(this);
    pagerRow->addWidget(prevBtn_);
    pagerRow->addWidget(pageLabel_);
    pagerRow->addWidget(nextBtn_);
    pagerRow->addStretch();
    root->addLayout(pagerRow);

    connect(addBtn, &QPushButton::clicked, this, &BirthdayPage::addBirthday);
    connect(editBtn, &QPushButton::clicked, this, &BirthdayPage::editSelected);
    connect(delBtn, &QPushButton::clicked, this, &BirthdayPage::deleteSelected);
    connect(prevBtn_, &QPushButton::clicked, this, &BirthdayPage::prevPage);
    connect(nextBtn_, &QPushButton::clicked, this, &BirthdayPage::nextPage);
    connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { editSelected(); });
}

void BirthdayPage::refresh() {
    items_ = BirthdayService().findAll();
    int pageCount = qMax(1, (items_.size() + kPageSize - 1) / kPageSize);
    page_ = qBound(0, page_, pageCount - 1);
    renderCards();
    pageLabel_->setText(QStringLiteral("\u7b2c %1 / %2 \u9875\uff0c\u5171 %3 \u4eba") // 第 X / Y 页，共 N 人
        .arg(page_ + 1).arg(pageCount).arg(items_.size()));
    prevBtn_->setEnabled(page_ > 0);
    nextBtn_->setEnabled(page_ < pageCount - 1);
}

QWidget* BirthdayPage::createCard(const models::Birthday& b) {
    BirthdayService svc;
    auto* card = new QWidget();
    card->setStyleSheet("QWidget#birthdayCard { background: #FFFFFF; border: 1px solid #E0E6EA; border-radius: 6px; }");
    card->setObjectName("birthdayCard");
    auto* h = new QHBoxLayout(card);
    h->setContentsMargins(10, 8, 10, 8);

    auto* avatar = new QLabel(card);
    QPixmap pix;
    if (!b.avatarPath.isEmpty() && pix.load(b.avatarPath)) {
        avatar->setPixmap(pix.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        avatar->setText(b.name.left(1));
        avatar->setFixedSize(50, 50);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet("background: #4FC3F7; color: white; font-size: 22px; border-radius: 25px;");
    }
    h->addWidget(avatar);

    auto* infoCol = new QVBoxLayout();
    auto* nameLabel = new QLabel(b.name + (b.gender == 1 ? QStringLiteral(" \u2640") : QStringLiteral(" \u2642")), card);
    nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #37474F;");
    QString dateText = b.isLunar
        ? QStringLiteral("\u519c\u5386 %1%2").arg(LunarCalendar::getLunarMonthName(b.lunarMonth)).arg(LunarCalendar::getLunarDayName(b.lunarDay)) // 农历 X月X日
        : QStringLiteral("\u516c\u5386 %1-%2-%3") // 公历 yyyy-MM-dd
              .arg(b.solarYear).arg(b.solarMonth, 2, 10, QLatin1Char('0')).arg(b.solarDay, 2, 10, QLatin1Char('0'));
    auto* dateLabel = new QLabel(dateText + QStringLiteral("  \u63d0\u9192 %1").arg(b.remindTime), card); // 提醒 HH:mm
    dateLabel->setStyleSheet("color: #78909C;");
    infoCol->addWidget(nameLabel);
    infoCol->addWidget(dateLabel);
    h->addLayout(infoCol, 1);

    int days = svc.daysUntilBirthday(b);
    QDate next = svc.nextBirthdayDate(b);
    auto* daysLabel = new QLabel(card);
    if (days == 0) {
        daysLabel->setText(QStringLiteral("\u4eca\u5929\u751f\u65e5\uff01")); // 今天生日！
        daysLabel->setStyleSheet("color: #E91E63; font-weight: bold; font-size: 15px;");
    } else {
        daysLabel->setText(QStringLiteral("\u8fd8\u6709 %1 \u5929\n%2").arg(days).arg(next.toString("MM-dd"))); // 还有 N 天
        daysLabel->setStyleSheet("color: #009688; font-weight: bold;");
    }
    daysLabel->setAlignment(Qt::AlignCenter);
    h->addWidget(daysLabel);

    card->setProperty("birthdayUuid", b.uuid);
    return card;
}

void BirthdayPage::renderCards() {
    list_->clear();
    int begin = page_ * kPageSize;
    int end = qMin(begin + kPageSize, items_.size());
    for (int i = begin; i < end; ++i) {
        auto* item = new QListWidgetItem(list_);
        auto* card = createCard(items_[i]);
        item->setSizeHint(QSize(0, 70));
        item->setData(Qt::UserRole, items_[i].uuid);
        list_->addItem(item);
        list_->setItemWidget(item, card);
    }
}

QString BirthdayPage_selectedUuid(QListWidget* list) {
    auto* item = list->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void BirthdayPage::addBirthday() {
    models::Birthday b;
    if (birthdayForm(this, b, true)) {
        BirthdayService().add(b);
        refresh();
        emit dataChanged();
    }
}

void BirthdayPage::editSelected() {
    QString uuid = BirthdayPage_selectedUuid(list_);
    if (uuid.isEmpty()) return;
    BirthdayService svc;
    auto b = svc.findByUuid(uuid);
    if (b.uuid.isEmpty()) return;
    if (birthdayForm(this, b, false)) {
        svc.update(b);
        refresh();
        emit dataChanged();
    }
}

void BirthdayPage::deleteSelected() {
    QString uuid = BirthdayPage_selectedUuid(list_);
    if (uuid.isEmpty()) return;
    if (QMessageBox::question(this, QStringLiteral("\u786e\u8ba4"),
            QStringLiteral("\u786e\u5b9a\u5220\u9664\u8be5\u751f\u65e5\uff1f")) == QMessageBox::Yes) { // 确定删除该生日？
        BirthdayService().remove(uuid);
        refresh();
        emit dataChanged();
    }
}

void BirthdayPage::prevPage() {
    if (page_ > 0) { --page_; renderCards(); refresh(); }
}

void BirthdayPage::nextPage() {
    int pageCount = qMax(1, (items_.size() + kPageSize - 1) / kPageSize);
    if (page_ < pageCount - 1) { ++page_; renderCards(); refresh(); }
}

} // namespace mcclock::gui
