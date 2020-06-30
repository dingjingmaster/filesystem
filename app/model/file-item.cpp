#include "file-item.h"

#include "file-item-model.h"

#include <file/file-info-manager.h>

FileItem::FileItem(std::shared_ptr<FileInfo> info, FileItem *parentItem, FileItemModel *model, QObject *parent) : QObject(parent)
{
    mParent = parentItem;
    mInfo = info;
    mChildren = new QVector<FileItem*>();
    mModel = model;
    mBackendEnumerator = new FileEnumerator(this);

    setParent(mModel);
}

FileItem::~FileItem()
{
    Q_EMIT cancelFindChildren();
    //disconnect();

    if (mInfo.use_count() <= 2) {
        auto info = FileInfoManager::getInstance()->findFileInfoByUri(mInfo->uri()).get();
        if (info == mInfo.get()) {
            FileInfoManager::getInstance()->lock();
            FileInfoManager::getInstance()->remove(mInfo);
            FileInfoManager::getInstance()->unlock();
        }
    }

    for (auto child : *mChildren) {
        delete child;
    }
    mChildren->clear();

    delete mChildren;
}

bool FileItem::hasChildren()
{

}

const QString FileItem::uri()
{
    return mInfo->uri();
}

void FileItem::findChildrenAsync()
{

}

QModelIndex FileItem::lastColumnIndex()
{

}

QModelIndex FileItem::firstColumnIndex()
{

}

QVector<FileItem *> *FileItem::findChildrenSync()
{

}

const std::shared_ptr<FileInfo> FileItem::info()
{
    return mInfo;
}

bool FileItem::operator ==(const FileItem &item)
{

}

void FileItem::clearChildren()
{

}

void FileItem::onUpdateDirectoryRequest()
{

}

void FileItem::onChildAdded(const QString &uri)
{

}

void FileItem::onDeleted(const QString &thisUri)
{

}

void FileItem::onChildRemoved(const QString &uri)
{

}

void FileItem::onRenamed(const QString &oldUri, const QString &newUri)
{

}

void FileItem::updateInfoSync()
{

}

void FileItem::updateInfoAsync()
{

}

FileItem *FileItem::getChildFromUri(QString uri)
{

}
