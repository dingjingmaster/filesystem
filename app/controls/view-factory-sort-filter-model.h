#ifndef VIEWFACTORYSORTFILTERMODEL_H
#define VIEWFACTORYSORTFILTERMODEL_H
#include <QObject>

#include <QSortFilterProxyModel>

class ViewFactorySortFilterModel2 : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ViewFactorySortFilterModel2(QObject *parent = nullptr);

    const QModelIndex getIndexFromViewId(const QString &viewId);
    const QString getHighestPriorityViewId(int zoom_level_hint);
    const QStringList supportViewIds();

    const QIcon iconFromViewId(const QString &viewId);
    const QString getViewDisplayNameFromId(const QString &viewId);

public Q_SLOTS:
    void setDirectoryUri(const QString &uri);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

#endif // VIEWFACTORYSORTFILTERMODEL_H
