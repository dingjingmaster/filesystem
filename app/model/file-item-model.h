#ifndef FILEITEMMODEL_H
#define FILEITEMMODEL_H

#include <QAbstractItemModel>

class FileItem;
class FileItemProxyFilterSortModel;

class FileItemModel : public QAbstractItemModel
{
    friend class FileItem;
    friend class FileItemProxyFilterSortModel;
    Q_OBJECT
public:
    enum ColumnType
    {
        FileName,
        FileSize,
        FileType,
        ModifiedDate,
        Owner,
        Other
    };
    Q_ENUM(ColumnType)

    enum ItemRole
    {
        UriRole = Qt::UserRole
    };
    Q_ENUM(ItemRole)

    explicit FileItemModel (QObject *parent = nullptr);
    ~FileItemModel () override;

    bool canExpandChildren ();
    bool isPositiveResponse ();
    const QString getRootUri ();
    void setRootItem (FileItem *item);
    void setExpandable (bool expandable);
    void setRootUri (const QString &uri);
    QModelIndex lastColumnIndex (FileItem *item);
    QModelIndex firstColumnIndex (FileItem *item);
    void setPositiveResponse (bool positive = true);
    void fetchMore (const QModelIndex &parent) override;
    const QModelIndex indexFromUri (const QString &uri);
    Qt::DropActions supportedDropActions () const override;
    int rowCount (const QModelIndex &parent) const override;
    FileItem *itemFromIndex (const QModelIndex &index) const;
    int columnCount (const QModelIndex &parent) const override;
    bool hasChildren (const QModelIndex &parent) const override;
    QModelIndex parent (const QModelIndex &child) const override;
    bool canFetchMore (const QModelIndex &parent) const override;
    Qt::ItemFlags flags (const QModelIndex &index) const override;
    QVariant data (const QModelIndex &index, int role) const override;
    QMimeData *mimeData (const QModelIndexList& indexes) const override;
    QModelIndex index (int row, int column, const QModelIndex &parent) const override;
    QVariant headerData (int section, Qt::Orientation orientation, int role) const override;
    bool insertRows (int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows (int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns (int column, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns (int column, int count, const QModelIndex &parent = QModelIndex()) override;
    bool dropMimeData (const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

Q_SIGNALS:
    void updated ();
    void findChildrenStarted ();
    void findChildrenFinished ();

public Q_SLOTS:
    void slotCancelFindChildren ();
    void slotOnItemAdded (FileItem *item);
    void slotOnItemRemoved (FileItem *item);
    void slotSetRootIndex (const QModelIndex &index);
    void slotOnFoundChildren (const QModelIndex &parent);

private:
    bool            mCanExpand = false;
    bool            mIsPositive = false;
    FileItem*       mRootItem = nullptr;
};

#endif // FILEITEMMODEL_H
