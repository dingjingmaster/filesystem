#ifndef SORTTYPEMENU_H
#define SORTTYPEMENU_H

#include <QMenu>

class SortTypeMenu : public QMenu
{
    Q_OBJECT
public:
    explicit SortTypeMenu(QWidget *parent = nullptr);

Q_SIGNALS:
    void switchSortTypeRequest(int type);
    void switchSortOrderRequest(Qt::SortOrder order);

public Q_SLOTS:
    void setSortType(int type);
    void setSortOrder(Qt::SortOrder order);

private:
    int mSortType = 0;
    Qt::SortOrder mSortOrder = Qt::AscendingOrder;

    QActionGroup *mSortTypes;
    QActionGroup *mSortOrders;
};


#endif // SORTTYPEMENU_H
