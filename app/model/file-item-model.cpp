#include "file-item-model.h"

#include "file-item.h"

bool FileItemModel::isPositiveResponse()
{
    return mIsPositive;
}

const QString FileItemModel::getRootUri()
{
    if (nullptr != mRootItem) {
        return mRootItem->uri();
    }
}

void FileItemModel::setRootItem(FileItem *item)
{

}

void FileItemModel::setPositiveResponse(bool positive)
{
    mIsPositive = positive;
}

void FileItemModel::fetchMore(const QModelIndex &parent)
{

}

const QModelIndex FileItemModel::indexFromUri(const QString &uri)
{

}

Qt::DropActions FileItemModel::supportedDropActions() const
{

}

int FileItemModel::rowCount(const QModelIndex &parent) const
{

}

FileItem *FileItemModel::itemFromIndex(const QModelIndex &index) const
{

}

int FileItemModel::columnCount(const QModelIndex &parent) const
{

}

bool FileItemModel::hasChildren(const QModelIndex &parent) const
{

}

QModelIndex FileItemModel::parent(const QModelIndex &child) const
{

}

bool FileItemModel::canFetchMore(const QModelIndex &parent) const
{

}

Qt::ItemFlags FileItemModel::flags(const QModelIndex &index) const
{

}

QVariant FileItemModel::data(const QModelIndex &index, int role) const
{

}

QMimeData *FileItemModel::mimeData(const QModelIndexList &indexes) const
{

}

QModelIndex FileItemModel::index(int row, int column, const QModelIndex &parent) const
{

}

QVariant FileItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{

}

bool FileItemModel::insertRows(int row, int count, const QModelIndex &parent)
{

}

bool FileItemModel::removeRows(int row, int count, const QModelIndex &parent)
{

}

bool FileItemModel::insertColumns(int column, int count, const QModelIndex &parent)
{

}

bool FileItemModel::removeColumns(int column, int count, const QModelIndex &parent)
{

}

bool FileItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{

}

FileItemModel::FileItemModel(QObject *parent) : QAbstractItemModel (parent)
{
    setPositiveResponse(true);
}

FileItemModel::~FileItemModel()
{
    disconnect();

    if (mRootItem) {
        delete mRootItem;
    }
}

bool FileItemModel::canExpandChildren()
{
    return  mCanExpand;
}

void FileItemModel::setExpandable(bool expandable)
{
    mCanExpand = expandable;
}

void FileItemModel::setRootUri(const QString &uri)
{

}

QModelIndex FileItemModel::lastColumnIndex(FileItem *item)
{

}

QModelIndex FileItemModel::firstColumnIndex(FileItem *item)
{

}

void FileItemModel::slotCancelFindChildren()
{

}

void FileItemModel::slotOnItemAdded(FileItem *item)
{

}

void FileItemModel::slotOnItemRemoved(FileItem *item)
{

}

void FileItemModel::slotSetRootIndex(const QModelIndex &index)
{

}

void FileItemModel::slotOnFoundChildren(const QModelIndex &parent)
{

}
