#include "sidebar-model.h"

#include "file-operation-utils.h"
#include "sidebar-personal-item.h"
#include "sidebar-favorite-item.h"
#include "sidebar-separator-item.h"
#include "sidebar-filesystem-item.h"

#include <QUrl>
#include <QIcon>
#include <qmimedata.h>

#include <file/file-info-job.h>
#include <file/file-info.h>

#include <window/bookmark-manager.h>


SideBarModel::SideBarModel(QObject *parent) : QAbstractItemModel (parent)
{
    beginResetModel();

    mRootChildren = new QVector<SideBarAbstractItem*>();
    SideBarFavoriteItem *favorite_root_item = new SideBarFavoriteItem(nullptr, nullptr, this);
    mRootChildren->append(favorite_root_item);

    SideBarPersonalItem *personal_root_item = new SideBarPersonalItem(nullptr, nullptr, this);
    mRootChildren->append(personal_root_item);

    SideBarFileSystemItem *computerItem = new SideBarFileSystemItem(nullptr, nullptr, this);
    mRootChildren->append(computerItem);

    endResetModel();

    connect(this, &SideBarModel::indexUpdated, this, &SideBarModel::onIndexUpdated);
}

SideBarModel::~SideBarModel()
{
    for (auto child : *mRootChildren) {
        delete child;
    }
    mRootChildren->clear();
    delete mRootChildren;
}

QModelIndex SideBarModel::lastCloumnIndex(SideBarAbstractItem *item)
{
    if (item->parent() != nullptr) {
        createIndex(item->parent()->mChildren->indexOf(item), 1, item);
    } else {
        for (auto child : *mRootChildren) {
            if (item->type() == child->type()) {
                return createIndex(mRootChildren->indexOf(child), 1, item);
            }
        }
    }
    return QModelIndex();
}

QModelIndex SideBarModel::firstCloumnIndex(SideBarAbstractItem *item)
{
    if (item->parent() != nullptr) {
        return createIndex(item->parent()->mChildren->indexOf(item), 0, item);
    } else {
        return createIndex(mRootChildren->indexOf(item), 0, item);
    }
}

SideBarAbstractItem *SideBarModel::itemFromIndex(const QModelIndex &index)
{
    return static_cast<SideBarAbstractItem*> (index.internalPointer());
}

QVariant SideBarModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();
}

bool SideBarModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    return false;
}

QModelIndex SideBarModel::parent(const QModelIndex &index) const
{
    SideBarAbstractItem *item = static_cast<SideBarAbstractItem*>(index.internalPointer());
    if (!item)
        return QModelIndex();
    if (item->parent())
        return item->parent()->firstColumnIndex();

    return QModelIndex();
}

int SideBarModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return mRootChildren->count();
    }

    SideBarAbstractItem *parentItem = static_cast<SideBarAbstractItem*>(parent.internalPointer());

    return parentItem->mChildren->count();
}

int SideBarModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QModelIndex SideBarModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, mRootChildren->at(row));
    }
    SideBarAbstractItem *parentItem = static_cast<SideBarAbstractItem*>(parent.internalPointer());
    if (parentItem->mChildren->count() > row) {
        return createIndex(row, column, parentItem->mChildren->at(row));
    }
    return QModelIndex();
}

void SideBarModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent);
}

bool SideBarModel::canFetchMore(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return true;
}

bool SideBarModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    }

    SideBarAbstractItem *parentItem = static_cast<SideBarAbstractItem*>(parent.internalPointer());

    return parentItem->hasChildren();
}

QVariant SideBarModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    SideBarAbstractItem *item = static_cast<SideBarAbstractItem*>(index.internalPointer());
    if (index.column() == 1) {
        if (role == Qt::DecorationRole && item->isMounted())
            return QVariant(QIcon::fromTheme("media-eject"));
        else {
            return QVariant();
        }
    }

    switch (role) {
    case Qt::DecorationRole:
        return QIcon::fromTheme("ukui-" + item->iconName(), QIcon::fromTheme(item->iconName()));
    case Qt::DisplayRole:
        return item->displayName();
    case Qt::ToolTipRole:
        return item->displayName();
    case Qt::UserRole:
        return item->uri();
    default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags SideBarModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;
}

bool SideBarModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        Q_EMIT dataChanged(index, index, QVector<int>() << role);
        return true;
    }

    return false;
}

bool SideBarModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    endInsertRows();

    return true;
}

bool SideBarModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    beginInsertColumns(parent, column, column + count - 1);
    endInsertColumns();

    return true;
}

Qt::DropActions SideBarModel::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction | Qt::LinkAction;
}

bool SideBarModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    endRemoveRows();

    return true;
}

bool SideBarModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    beginRemoveColumns(parent, column, column + count - 1);
    endRemoveColumns();

    return true;
}

bool SideBarModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (data->urls().isEmpty()) {
        return false;
    }

    auto item = this->itemFromIndex(parent);
    if (!parent.isValid()) {

        auto bookmark = BookMarkManager::getInstance();
        if (bookmark->isLoaded()) {
            for (auto url : data->urls()) {
                auto info = FileInfo::fromUri(url.toDisplayString(), false);
                if (info->displayName().isNull()) {
                    FileInfoJob j(info);
                    j.querySync();
                }
                if (info->isDir()) {
                    bookmark->addBookMark(url.url());
                }
            }
        }
        return true;
    }

    switch (item->type()) {
    case SideBarAbstractItem::SeparatorItem:
    case SideBarAbstractItem::FavoriteItem: {
        auto bookmark = BookMarkManager::getInstance();
        if (bookmark->isLoaded()) {
            for (auto url : data->urls()) {
                auto info = FileInfo::fromUri(url.toDisplayString(), false);
                if (info->displayName().isNull()) {
                    FileInfoJob j(info);
                    j.querySync();
                }
                if (info->isDir()) {
                    bookmark->addBookMark(url.url());
                }
            }
        }
        break;
    }
    case SideBarAbstractItem::PersonalItem:
    case SideBarAbstractItem::FileSystemItem: {
        QStringList uris;
        for (auto url : data->urls()) {
            uris<<url.url();
        }
        FileOperationUtils::move(uris, item->uri(), true, true);
        break;
    }
    }
    return true;
}

void SideBarModel::onIndexUpdated(const QModelIndex &index)
{
    auto item = itemFromIndex(index);
    bool isEmpty = true;
    for (auto child : *item->mChildren) {
        auto info = FileInfo::fromUri(child->uri());
        if (!info->displayName().startsWith(".") && (info->isDir() || info->isVolume()))
            isEmpty = false;
        if (child->type() == SideBarAbstractItem::SeparatorItem) {
            removeRows(item->mChildren->indexOf(child), 1, index);
            item->mChildren->removeOne(child);
        }
    }
    if (isEmpty) {
        auto separator = new SideBarSeparatorItem(SideBarSeparatorItem::EmptyFile, item, this);
        item->mChildren->append(separator);
        insertRows(item->mChildren->count() - 1, 1, index);
    }
}
