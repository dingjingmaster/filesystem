#include "file-info-manager.h"

#include <QHash>
#include <clib_syslog.h>
#include <thumbnail-manager.h>

static FileInfoManager* gFileInfoManager = nullptr;
static QHash<QString, std::shared_ptr<FileInfo>> *gInfoList = nullptr;

void FileInfoManager::lock()
{
    mMutex.tryLock();
}

void FileInfoManager::clear()
{
    Q_ASSERT(gInfoList);
    gInfoList->clear();
}

void FileInfoManager::unlock()
{
    mMutex.unlock();
}

void FileInfoManager::showState()
{
    //gInfoList->keys().count() << gInfoList->values().count();
}

void FileInfoManager::remove(QString uri)
{
    ThumbnailManager::getInstance()->releaseThumbnail(uri);
    Q_ASSERT(gInfoList);
    gInfoList->remove(uri);
}

FileInfoManager *FileInfoManager::getInstance()
{
    if (!gFileInfoManager) {
        gFileInfoManager = new FileInfoManager;
    }

    return gFileInfoManager;
}

void FileInfoManager::remove(std::shared_ptr<FileInfo> info)
{
    Q_ASSERT(gInfoList);
    this->remove(info->uri());
}

std::shared_ptr<FileInfo> FileInfoManager::findFileInfoByUri(QString uri)
{
    Q_ASSERT(gInfoList);
    return gInfoList->value(uri);
}

void FileInfoManager::removeFileInfobyUri(QString uri)
{
    Q_ASSERT(gInfoList);
    gInfoList->remove(uri);
}

std::shared_ptr<FileInfo> FileInfoManager::insertFileInfo(std::shared_ptr<FileInfo> info)
{
    Q_ASSERT(gInfoList);

    if (gInfoList->value(info->uri())) {
        info = gInfoList->value(info->uri());
    } else {
        gInfoList->insert(info->uri(), info);
    }

    return info;
}

FileInfoManager::FileInfoManager()
{
    gInfoList = new QHash <QString, std::shared_ptr<FileInfo>> ();
}

FileInfoManager::~FileInfoManager()
{
    delete gInfoList;
}
