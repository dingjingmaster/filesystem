#include "sidebar-proxy-filter-sort-model.h"

#include "sidebar-model.h"

SideBarProxyFilterSortModel::SideBarProxyFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent)
{

}

SideBarAbstractItem *SideBarProxyFilterSortModel::itemFromIndex(const QModelIndex &proxyIndex)
{
    SideBarModel *model = static_cast<SideBarModel*>(sourceModel());
    auto index = mapToSource(proxyIndex);
    return model->itemFromIndex(index);
}

bool SideBarProxyFilterSortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!(left.isValid() && right.isValid())) {
        return QSortFilterProxyModel::lessThan(left, right);
    }
    auto leftItem = static_cast<SideBarAbstractItem*>(left.internalPointer());
    auto rightItem = static_cast<SideBarAbstractItem*>(right.internalPointer());
    if (leftItem->type() != SideBarAbstractItem::FileSystemItem ||
            rightItem->type() != SideBarAbstractItem::FileSystemItem) {
        return false;
    }

    return leftItem->displayName().compare(rightItem->displayName()) > 0;
}

bool SideBarProxyFilterSortModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    auto item = static_cast<SideBarAbstractItem*>(index.internalPointer());
    if (item->type() != SideBarAbstractItem::SeparatorItem) {
        if (item->displayName() == "" || item->displayName().isNull()) {
            return false;
        }
    }
    if (item) {
        if (!item->displayName().isEmpty()) {
            QString dn = item->displayName();
            if (dn.length() > 0 && dn.at(0) == ".") {
                return false;
            }
        }
    }
    if (item->type() == SideBarAbstractItem::FileSystemItem) {
        if (sourceParent.data(Qt::UserRole).toString() == "computer:///") {
            if (item->uri() != "computer:///root.link") {
                if (!item->isRemoveable() && !item->isEjectable())
                    return true;
                if (/*!item->isMountable() || */!item->isMounted()) {
                    return false;
                }
            }
        }
    }
    return true;
}
