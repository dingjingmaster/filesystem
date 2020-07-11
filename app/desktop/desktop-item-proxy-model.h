#ifndef DESKTOPITEMPROXYMODEL_H
#define DESKTOPITEMPROXYMODEL_H

#include <QSortFilterProxyModel>



class DesktopItemProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum SortType
    {
        FileName,
        FileType,
        FileSize,
        ModifiedDate,
        Other
    };
    explicit DesktopItemProxyModel(QObject *parent = nullptr);

    int getSortType();
    void setSortType (int type);
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

private:
    int mSortType = Other;
};

#endif // DESKTOPITEMPROXYMODEL_H
