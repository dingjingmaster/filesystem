#include "file-item-model.h"
#include "file-item.h"

#include <QUrl>
#include <QIcon>
#include <QMimeData>
#include <file-utils.h>
#include <thumbnail-manager.h>
#include <syslog/clib_syslog.h>
#include <file-operation-utils.h>
#include <file/file-copy-operation.h>
#include <file/file-operation-manager.h>

bool FileItemModel::isPositiveResponse()
{
    return mIsPositive;
}

const QString FileItemModel::getRootUri()
{
    if (nullptr != mRootItem) {
        return mRootItem->uri();
    }

    CT_SYSLOG(LOG_DEBUG, "uri is nullptr");
    return nullptr;
}

void FileItemModel::setRootItem(FileItem *item)
{
    beginResetModel();
    mRootItem->deleteLater();

    mRootItem = item;
    mRootItem->findChildrenAsync();

    endResetModel();
}

void FileItemModel::setPositiveResponse(bool positive)
{
    mIsPositive = positive;
}

void FileItemModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent);
}

const QModelIndex FileItemModel::indexFromUri(const QString &uri)
{
    for (auto child : *mRootItem->mChildren) {
        if (child->uri() == uri) {
            return child->firstColumnIndex();
        }
    }
    return QModelIndex();
}

Qt::DropActions FileItemModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

int FileItemModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (!mRootItem) {
            return 0;
        }
        return mRootItem->mChildren->count();
    }
    FileItem *parent_item = static_cast<FileItem*>(parent.internalPointer());
    return parent_item->mChildren->count();
}

FileItem *FileItemModel::itemFromIndex(const QModelIndex &index) const
{
    return static_cast<FileItem*>(index.internalPointer());
}

int FileItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return ModifiedDate + 1;
}

bool FileItemModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    }
    FileItem *parent_item = static_cast<FileItem*>(parent.internalPointer());
    if (parent_item->hasChildren() && mCanExpand) {
        return true;
    }
    return false;
}

QModelIndex FileItemModel::parent(const QModelIndex &child) const
{
    FileItem *childItem = static_cast<FileItem*>(child.internalPointer());
    if (childItem->mParent == nullptr) {
        return QModelIndex();
    }

    return childItem->mParent->firstColumnIndex();
}

bool FileItemModel::canFetchMore(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    }
    FileItem *parent_item = static_cast<FileItem*>(parent.internalPointer());
    if (!parent_item->mExpanded) {
        return true;
    }
    return false;
}

Qt::ItemFlags FileItemModel::flags(const QModelIndex &index) const
{
    if (index.isValid()) {
        Qt::ItemFlags flags = QAbstractItemModel::flags(index);

        auto item = itemFromIndex(index);
        if (item->mInfo->isDir()) {
            flags |= Qt::ItemIsDropEnabled;
        }
        if (index.column() == FileName) {
            flags |= Qt::ItemIsDragEnabled;
            flags |= Qt::ItemIsEditable;
        }
        return flags;
    } else {
        return Qt::ItemIsDropEnabled;
    }
}

QVariant FileItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    FileItem *item = static_cast<FileItem*>(index.internalPointer());

    if (role == FileItemModel::UriRole) {
        return QVariant(item->uri());
    }

    switch (index.column()) {
    case FileName: {
        switch (role) {
        case Qt::TextAlignmentRole: {
            return QVariant(Qt::AlignHCenter | Qt::AlignBaseline);
        }
        case Qt::DisplayRole: {
            return QVariant(item->mInfo->displayName());
        }
        case Qt::DecorationRole: {
            auto thumbnail = ThumbnailManager::getInstance()->tryGetThumbnail(item->mInfo->uri());
            if (!thumbnail.isNull()) {
                if (item->mInfo->uri().endsWith(".desktop") && !item->mInfo->canExecute()) {
                    return QIcon::fromTheme(item->mInfo->iconName(), QIcon::fromTheme("text-x-generic"));
                }
                return thumbnail;
            }
            QIcon icon = QIcon::fromTheme(item->mInfo->iconName(), QIcon::fromTheme("text-x-generic"));
            return QVariant(icon);
        }
        case Qt::ToolTipRole: {
            return QVariant(item->mInfo->displayName());
        }
        default:
            return QVariant();
        }
    }
    case FileSize: {
        switch (role) {
        case Qt::DisplayRole: {
            if (item->hasChildren()) {
                if (item->mExpanded) {
                    return QVariant(QString::number(item->mChildren->count()) + tr("child(ren)"));
                }
                return QVariant();
            }
            return QVariant(item->mInfo->fileSize());
        }
        default:
            return QVariant();
        }
    }
    case FileType:
        switch (role) {
        case Qt::DisplayRole: {
            if (item->mInfo->isSymbolLink()) {
                return QVariant(tr("Symbol Link, ") + item->mInfo->fileType());
            }
            return QVariant(item->mInfo->fileType());
        }
        default:
            return QVariant();
        }
    case ModifiedDate: {
        switch (role) {
        case Qt::DisplayRole:
            return QVariant(item->mInfo->modifiedDate());
        default:
            return QVariant();
        }
    }
    default:
        return QVariant();
    }
}

QMimeData *FileItemModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData* data = QAbstractItemModel::mimeData(indexes);
    QList<QUrl> urls;
    for (auto index : indexes) {
        auto item = itemFromIndex(index);
        QUrl url = item->mInfo->uri();
        urls << url;
    }

    data->setUrls(urls);
    return data;
}

QModelIndex FileItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row < 0 || row > mRootItem->mChildren->count() - 1) {
            return QModelIndex();
        }

        return createIndex(row, column, mRootItem->mChildren->at(row));
    }

    FileItem *item = static_cast<FileItem*>(parent.internalPointer());
    if (row < 0 || row > item->mChildren->count() - 1) {
        return QModelIndex();
    }

    return createIndex(row, column, item->mChildren->at(row));
}

QVariant FileItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        switch (section) {
        case FileName:
            return tr("File Name");
        case FileSize:
            return tr("File Size");
        case FileType:
            return tr("File Type");
        case ModifiedDate:
            return tr("Modified Date");
        default:
            return QVariant();
        }
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

bool FileItemModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endInsertRows();
    return true;
}

bool FileItemModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endRemoveRows();
    return true;
}

bool FileItemModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    beginInsertColumns(parent, column, column + count - 1);
    // FIXME: Implement me!
    endInsertColumns();
    return true;
}

bool FileItemModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    beginRemoveColumns(parent, column, column + count - 1);
    // FIXME: Implement me!
    endRemoveColumns();
    return true;
}

bool FileItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    QString destDirUri = nullptr;
    if (parent.isValid()) {
        QModelIndex child = index(row, column, parent);
        if (child.isValid()) {
        } else {
            auto parentItem = itemFromIndex(parent);
            destDirUri = parentItem->mInfo->uri();
        }
    } else {
        destDirUri = mRootItem->mInfo->uri();
        auto targetUri = FileUtils::getTargetUri(destDirUri);
        if (!targetUri.isEmpty()) {
            destDirUri = targetUri;
        }
    }

    if (destDirUri.isNull()) {
        return false;
    }

    if (!FileUtils::getFileIsFolder(destDirUri)) {
        return false;
    }

    auto urls = data->urls();
    if (urls.isEmpty()) {
        return false;
    }

    QStringList srcUris;
    for (auto url : urls) {
        srcUris<<url.url();
    }

    if (srcUris.contains(destDirUri)) {
        return false;
    }

    auto fileOpMgr = FileOperationManager::getInstance();
    bool addHistory = true;
    switch (action) {
    case Qt::MoveAction: {
        FileOperationUtils::move(srcUris, destDirUri, addHistory, true);
        break;
    }
    case Qt::CopyAction: {
        FileCopyOperation *copyOp = new FileCopyOperation(srcUris, destDirUri);
        fileOpMgr->slotStartOperation(copyOp);
        break;
    }
    default:
        break;
    }

    return true;
}

FileItemModel::FileItemModel(QObject *parent) : QAbstractItemModel (parent)
{
    CT_SYSLOG(LOG_DEBUG, "FileItemModel construct ...");
    setPositiveResponse(true);
    CT_SYSLOG(LOG_DEBUG, "FileItemModel construct ok!");
}

FileItemModel::~FileItemModel()
{
    CT_SYSLOG(LOG_DEBUG, "FileItemModel destroy ...");
    disconnect();

    if (mRootItem) {
        delete mRootItem;
    }
    CT_SYSLOG(LOG_DEBUG, "FileItemModel destroy ok!");
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
    if (uri.isNull()) {
        setRootUri("file:///");
        return;
    }

    auto info = FileInfo::fromUri(uri);
    auto item = new FileItem(info, nullptr, this);

    setRootItem(item);
}

QModelIndex FileItemModel::lastColumnIndex(FileItem *item)
{
    if (!item->mParent) {
        for (int i = 0; i < mRootItem->mChildren->count(); i++) {
            if (item == mRootItem->mChildren->at(i)) {
                return createIndex(i, Other, item);
            }
        }
        return QModelIndex();
    } else {
        for (int i = 0; i < item->mParent->mChildren->count(); i++) {
            if (item == item->mParent->mChildren->at(i)) {
                return createIndex(i, Other, item);
            }
        }
        return QModelIndex();
    }
}

QModelIndex FileItemModel::firstColumnIndex(FileItem *item)
{
    if (item->mParent == nullptr) {
        for (int i = 0; i < mRootItem->mChildren->count(); i++) {
            if (item == mRootItem->mChildren->at(i)) {
                return createIndex(i, 0, item);
            }
        }
        return QModelIndex();
    } else {
        for (int i = 0; i < item->mParent->mChildren->count(); i++) {
            if (item == item->mParent->mChildren->at(i)) {
                return createIndex(i, 0, item);
            }
        }
        return QModelIndex();
    }
}

void FileItemModel::slotCancelFindChildren()
{
    mRootItem->cancelFindChildren();
}

void FileItemModel::slotOnItemAdded(FileItem *item)
{
    if (!item->mParent) {
        insertRow(item->firstColumnIndex().row());
    }
    insertRow(item->firstColumnIndex().row(), item->mParent->firstColumnIndex());
}

void FileItemModel::slotOnItemRemoved(FileItem *item)
{
    if (!item->mParent) {
        removeRow(item->firstColumnIndex().row());
    }
    removeRow(item->firstColumnIndex().row(), item->mParent->firstColumnIndex());
}

void FileItemModel::slotSetRootIndex(const QModelIndex &index)
{
    if (index.isValid()) {
        auto new_root_info = FileInfo::fromUri(index.data(UriRole).toString());
        auto new_root_item = new FileItem(new_root_info, nullptr, this);
        if (new_root_item->hasChildren()) {
            setRootItem(new_root_item);
        }
    }
}

void FileItemModel::slotOnFoundChildren(const QModelIndex &parent)
{
    if (!parent.isValid()) {
        return;
    }
    FileItem *parentItem = static_cast<FileItem*>(parent.internalPointer());
    beginInsertRows(parent, 0, parentItem->mChildren->count() - 1);
    endInsertRows();
}
