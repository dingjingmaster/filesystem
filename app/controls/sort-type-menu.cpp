#include "sort-type-menu.h"

#include <QDebug>

SortTypeMenu::SortTypeMenu(QWidget *parent) : QMenu(parent)
{
    auto sortTypeGroup = new QActionGroup(this);
    mSortTypes = sortTypeGroup;
    sortTypeGroup->setExclusive(true);

    auto fileName = addAction(tr("File Name"));
    fileName->setCheckable(true);
    sortTypeGroup->addAction(fileName);

    auto fileSize = addAction(tr("File Size"));
    fileSize->setCheckable(true);
    sortTypeGroup->addAction(fileSize);

    auto fileType = addAction(tr("File Type"));
    fileType->setCheckable(true);
    sortTypeGroup->addAction(fileType);

    auto modifiedDate = addAction(tr("Modified Date"));
    modifiedDate->setCheckable(true);
    sortTypeGroup->addAction(modifiedDate);

    connect(sortTypeGroup, &QActionGroup::triggered, this, [=](QAction *action) {
        int index = sortTypeGroup->actions().indexOf(action);
        switchSortTypeRequest(index);
    });

    addSeparator();

    auto sortOrderGroup = new QActionGroup(this);
    mSortOrders = sortOrderGroup;
    sortOrderGroup->setExclusive(true);

    auto ascending = addAction(tr("Ascending"));
    ascending->setCheckable(true);
    sortOrderGroup->addAction(ascending);

    auto descending = addAction(tr("Descending"));
    descending->setCheckable(true);
    sortOrderGroup->addAction(descending);

    connect(sortOrderGroup, &QActionGroup::triggered, this, [=](QAction *action) {
        int index = sortOrderGroup->actions().indexOf(action);
        switchSortOrderRequest(Qt::SortOrder(index));
    });
}

void SortTypeMenu::setSortType(int type)
{
    mSortTypes->actions().at(type)->setChecked(true);
    qDebug() << mSortType<<type;
    if (mSortType != type) {
        mSortType = type;
    }
}

void SortTypeMenu::setSortOrder(Qt::SortOrder order)
{
    qDebug()<<mSortOrder<<order;
    if (order < 0)
        return;
    mSortOrders->actions().at(order)->setChecked(true);
    if (mSortOrder != order) {
        mSortOrder = order;
    }
}
