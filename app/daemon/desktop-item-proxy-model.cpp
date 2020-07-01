#include "desktop-item-proxy-model.h"

DesktopItemProxyModel::DesktopItemProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{

}

void DesktopItemProxyModel::setSortType(int type)
{
    mSortType = type;
}

bool DesktopItemProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{

}

bool DesktopItemProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{

}

int DesktopItemProxyModel::getSortType()
{
    return mSortType;
}
