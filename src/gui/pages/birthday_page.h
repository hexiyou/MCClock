#pragma once

#include <QWidget>
#include "core/models/all_models.h"

class QListWidget;
class QPushButton;
class QLabel;

namespace mcclock::gui {

// Birthday page: card-style list with pagination.
class BirthdayPage : public QWidget {
    Q_OBJECT
public:
    explicit BirthdayPage(QWidget* parent = nullptr);
    void refresh();

signals:
    void dataChanged();

private slots:
    void addBirthday();
    void editSelected();
    void deleteSelected();
    void prevPage();
    void nextPage();

private:
    void setupUi();
    void renderCards();
    QWidget* createCard(const models::Birthday& b);

    QListWidget* list_ = nullptr;
    QLabel* pageLabel_ = nullptr;
    QPushButton* prevBtn_ = nullptr;
    QPushButton* nextBtn_ = nullptr;
    QList<models::Birthday> items_;
    int page_ = 0;
    static constexpr int kPageSize = 6;
};

} // namespace mcclock::gui
