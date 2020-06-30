#ifndef SIDEBARMODEL_H
#define SIDEBARMODEL_H

#include "sidebar-abstract-item.h"

#include <QVector>
#include <QAbstractItemModel>

class SideBarAbstractItem;

class SideBarModel : public QAbstractItemModel
{
    friend class SideBarAbstractItem;
    Q_OBJECT
public:
    explicit SideBarModel (QObject *parent = nullptr);
    ~SideBarModel () override;

    QModelIndex lastCloumnIndex (SideBarAbstractItem *item);
    QModelIndex firstCloumnIndex (SideBarAbstractItem *item);
    SideBarAbstractItem *itemFromIndex (const QModelIndex &index);

    // Header:
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData (int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

    // Basic functionality:
    QModelIndex parent (const QModelIndex &index) const override;
    int rowCount (const QModelIndex &parent = QModelIndex()) const override;
    int columnCount (const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index (int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    // Fetch data dynamically:
    void fetchMore (const QModelIndex &parent) override;
    bool canFetchMore (const QModelIndex &parent) const override;
    bool hasChildren (const QModelIndex &parent = QModelIndex()) const override;
    QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    Qt::ItemFlags flags (const QModelIndex& index) const override;
    bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // Add data:
    bool insertRows (int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns (int column, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    Qt::DropActions supportedDropActions () const override;
    bool removeRows (int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns (int column, int count, const QModelIndex &parent = QModelIndex()) override;
    bool dropMimeData (const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

Q_SIGNALS:
    void indexUpdated (const QModelIndex &index);

protected:
    void onIndexUpdated (const QModelIndex &index);

protected:
    QStringList mBookmarkUris;
    QVector<SideBarAbstractItem*> *mRootChildren = nullptr;
};

#endif // SIDEBARMODEL_H
