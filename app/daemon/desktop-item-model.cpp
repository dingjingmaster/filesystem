#include "desktop-item-model.h"

#include <QUrl>
#include <QIcon>
#include <QMimeData>
#include <clib_syslog.h>
#include <QStandardPaths>
#include <file/file-info.h>
#include <file/file-watcher.h>
#include <file/file-info-job.h>
#include <file/file-enumerator.h>
#include <file/file-info-manager.h>
#include <file/file-move-operation.h>
#include <file/file-trash-operation.h>
#include <file/file-operation-manager.h>


DesktopItemModel::DesktopItemModel(QObject *parent) : QAbstractListModel(parent)
{
    CT_SYSLOG (LOG_DEBUG, "DesktopItemModel construct ...");
    mThumbnailWatcher = std::make_shared<FileWatcher>("thumbnail:///, this");
    CT_SYSLOG (LOG_DEBUG, "--------------------------------------------");

    connect(mThumbnailWatcher.get(), &FileWatcher::fileChanged, this, [=](const QString &uri) {
        for (auto info : mFiles) {
            if (info->uri() == uri) {
                auto index = indexFromUri(uri);
                Q_EMIT this->dataChanged(index, index);
            }
        }
    });

    mTrashWatcher = std::make_shared<FileWatcher>("trash:///", this);

    this->connect(mTrashWatcher.get(), &FileWatcher::fileCreated, [=]() {
        auto trash = FileInfo::fromUri("trash:///", true);
        auto job = new FileInfoJob(trash);
        job->setAutoDelete();
        connect(job, &FileInfoJob::infoUpdated, [=]() {
            auto trashIndex = this->indexFromUri("trash:///");
            this->dataChanged(trashIndex, trashIndex);
            Q_EMIT this->requestClearIndexWidget();
        });
        job->queryAsync();
    });

    this->connect(mTrashWatcher.get(), &FileWatcher::fileDeleted, [=]() {
        auto trash = FileInfo::fromUri("trash:///", true);
        auto job = new FileInfoJob(trash);
        job->setAutoDelete();
        connect(job, &FileInfoJob::infoUpdated, [=]() {
            auto trashIndex = this->indexFromUri("trash:///");
            this->dataChanged(trashIndex, trashIndex);
            Q_EMIT this->requestClearIndexWidget();
        });
        job->queryAsync();
    });

    mDesktopWatcher = std::make_shared<FileWatcher>("file://" + QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), this);
    mDesktopWatcher->setMonitorChildrenChange(true);
    this->connect(mDesktopWatcher.get(), &FileWatcher::fileCreated, [=](const QString &uri) {
        auto info = FileInfo::fromUri(uri, true);
        bool exsited = false;
        for (auto file : mFiles) {
            if (file->uri() == info->uri()) {
                exsited = true;
                break;
            }
        }

        if (!exsited) {
            auto job = new FileInfoJob(info);
            job->setAutoDelete();
            connect(job, &FileInfoJob::infoUpdated, [=]() {
                //this->beginResetModel();
                this->beginInsertRows(QModelIndex(), mFiles.count(), mFiles.count());
//                ThumbnailManager::getInstance()->createThumbnail(info->uri(), mDesktopWatcher);
                mFiles<<info;
                //this->insertRows(mFiles.indexOf(info), 1);
                this->endInsertRows();
                //this->endResetModel();
                Q_EMIT this->requestUpdateItemPositions();
                Q_EMIT this->requestLayoutNewItem(info->uri());
                Q_EMIT this->fileCreated(uri);
            });
            job->queryAsync();
        }
    });

    this->connect(mDesktopWatcher.get(), &FileWatcher::fileDeleted, [=](const QString &uri) {
        for (auto info : mFiles) {
            if (info->uri() == uri) {
                //this->beginResetModel();
                this->beginRemoveRows(QModelIndex(), mFiles.indexOf(info), mFiles.indexOf(info));
                mFiles.removeOne(info);
                this->endRemoveRows();
                //this->endResetModel();
                Q_EMIT this->requestClearIndexWidget();
                Q_EMIT this->requestUpdateItemPositions();
                FileInfoManager::getInstance()->remove(info);
            }
        }
    });

    this->connect(mDesktopWatcher.get(), &FileWatcher::fileChanged, [=](const QString &uri) {
        for (auto info : mFiles) {
            if (info->uri() == uri) {
                auto job = new FileInfoJob(info);
                job->setAutoDelete();
                connect(job, &FileInfoJob::queryAsyncFinished, this, [=]() {
//                    ThumbnailManager::getInstance()->createThumbnail(uri, mThumbnailWatcher);
                    this->dataChanged(indexFromUri(uri), indexFromUri(uri));
                    Q_EMIT this->requestClearIndexWidget();
                });
                job->queryAsync();
                this->dataChanged(indexFromUri(uri), indexFromUri(uri));
                return;
            }
        }
    });
    CT_SYSLOG (LOG_DEBUG, "DesktopItemModel construct successful!");
}

DesktopItemModel::~DesktopItemModel()
{
    FileInfoManager::getInstance()->clear();
}

const QString DesktopItemModel::indexUri(const QModelIndex &index)
{
    if (index.row() < 0 || index.row() >= mFiles.count()) {
        return nullptr;
    }
    return mFiles.at(index.row())->uri();
}

const QModelIndex DesktopItemModel::indexFromUri(const QString &uri)
{
    for (auto info : mFiles) {
        if (info->uri() == uri) {
            return index(mFiles.indexOf(info));
        }
    }
    return QModelIndex();
}

Qt::DropActions DesktopItemModel::supportedDropActions() const
{
    return QAbstractItemModel::supportedDropActions();
}

Qt::ItemFlags DesktopItemModel::flags(const QModelIndex &index) const
{
    auto uri = index.data(UriRole).toString();
    auto info = FileInfo::fromUri(uri, false);
    if (index.isValid()) {
        Qt::ItemFlags flags = QAbstractItemModel::flags(index);
        flags |= Qt::ItemIsDragEnabled;
        flags |= Qt::ItemIsEditable;
        if (info->isDir()) {
            flags |= Qt::ItemIsDropEnabled;
        }
        return flags;
    } else {
        return Qt::ItemIsDropEnabled;
    }
}

QMimeData *DesktopItemModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData* data = QAbstractItemModel::mimeData(indexes);
    QList<QUrl> urls;
    for (auto index : indexes) {
        QUrl url = index.data(UriRole).toString();
        urls<<url;
    }
    data->setUrls(urls);
    return data;
}

bool DesktopItemModel::removeRow(int row, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row);
    endRemoveRows();
    return true;
}

bool DesktopItemModel::insertRow(int row, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row);
    endInsertRows();
    return true;
}

int DesktopItemModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return mFiles.count();
}

QVariant DesktopItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    auto info = mFiles.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return info->displayName();
    case Qt::ToolTipRole:
        return info->displayName();
    case Qt::DecorationRole: {
//        auto thumbnail = info->thumbnail();
//        auto thumbnail = ThumbnailManager::getInstance()->tryGetThumbnail(info->uri());
//        if (!thumbnail.isNull()) {
//            if (info->uri().endsWith(".desktop") && !info->canExecute()) {
//                return QIcon::fromTheme(info->iconName(), QIcon::fromTheme("text-x-generic"));
//            }
//            return thumbnail;
//        }
//        return QIcon::fromTheme(info->iconName(), QIcon::fromTheme("text-x-generic"));
    }
    case UriRole:
        return info->uri();
    case IsLinkRole:
        return info->isSymbolLink();
    }
    return QVariant();
}

bool DesktopItemModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count);
    endInsertRows();
    return true;
}

bool DesktopItemModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count);
    endRemoveRows();
    return true;
}

bool DesktopItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    QString destDirUri = nullptr;
    if (parent.isValid()) {
        destDirUri = parent.data(UriRole).toString();
    } else {
        destDirUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }

    if (destDirUri.isNull()) {
        return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
    }

    auto info = FileInfo::fromUri(destDirUri);
    if (!info->isDir()) {
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
        return true;
    }

    auto fileOpMgr = FileOperationManager::getInstance();
    bool addHistory = true;
    if (destDirUri == "trash:///") {
        FileTrashOperation *trashOp = new FileTrashOperation(srcUris);
        fileOpMgr->slotStartOperation(trashOp, addHistory);
    } else {
        FileMoveOperation *moveOp = new FileMoveOperation(srcUris, destDirUri);
        moveOp->setCopyMove(true);
        fileOpMgr->slotStartOperation(moveOp, addHistory);
    }

    return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
}

void DesktopItemModel::refresh()
{
//    ThumbnailManager::getInstance()->syncThumbnailPreferences();
    beginResetModel();
    //removeRows(0, m_files.count());
    FileInfoManager::getInstance()->clear();
    //m_trash_watcher->stopMonitor();
    //m_desktop_watcher->stopMonitor();
    for (auto info : mFiles) {
//        ThumbnailManager::getInstance()->releaseThumbnail(info->uri());
    }
    mFiles.clear();

    mEnumerator = new FileEnumerator(this);
    mEnumerator->setAutoDelete();
    mEnumerator->setEnumerateDirectory("file://" + QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    mEnumerator->connect(mEnumerator, &FileEnumerator::enumerateFinished, this, &DesktopItemModel::onEnumerateFinished);
    mEnumerator->slotEnumerateAsync();
    endResetModel();
}

void DesktopItemModel::onEnumerateFinished()
{
    FileInfoManager::getInstance()->clear();
    mFiles.clear();

    auto computer = FileInfo::fromUri("computer:///", true);
    auto personal = FileInfo::fromPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), true);
    auto trash = FileInfo::fromUri("trash:///", true);

    QList<std::shared_ptr<FileInfo>> infos;

    infos << computer;
    infos << trash;
    infos << personal;

    infos << mEnumerator->getChildren(true);

    for (auto info : infos) {
        mInfoQueryQueue<<info->uri();
        mFiles<<info;

        auto job = new FileInfoJob(info);

        connect(job, &FileInfoJob::queryAsyncFinished, [=]() {
            this->dataChanged(this->index(mFiles.indexOf(info)), this->index(mFiles.indexOf(info)));
//            ThumbnailManager::getInstance()->createThumbnail(info->uri(), mDesktopWatcher);

            mInfoQueryQueue.removeOne(info->uri());
            if (mInfoQueryQueue.isEmpty()) {
                this->beginResetModel();
                this->endResetModel();
                Q_EMIT this->refreshed();
            }
        });

        connect(job, &FileInfoJob::infoUpdated, [=](){
            auto index = indexFromUri(info->uri());
            this->dataChanged(index, index);
        });

        job->setAutoDelete();
        job->queryAsync();
    }

    mTrashWatcher->startMonitor();
    mDesktopWatcher->startMonitor();
}
