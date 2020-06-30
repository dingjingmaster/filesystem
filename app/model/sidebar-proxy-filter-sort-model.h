#ifndef SIDEBARPROXYFILTERSORTMODEL_H
#define SIDEBARPROXYFILTERSORTMODEL_H

#include <QSortFilterProxyModel>

class SideBarAbstractItem;

class SideBarProxyFilterSortModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SideBarProxyFilterSortModel (QObject *parent = nullptr);
    SideBarAbstractItem *itemFromIndex (const QModelIndex &proxyIndex);

protected:
    bool lessThan (const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsRow (int sourceRow, const QModelIndex &sourceParent) const override;
};

#endif // SIDEBARPROXYFILTERSORTMODEL_H
