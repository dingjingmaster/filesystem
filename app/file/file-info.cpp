#include "file-info-manager.h"

#include <QUrl>
#include <memory>
#include "file-info.h"
#include <clib_syslog.h>

FileInfo::FileInfo(QObject *parent) : QObject(parent)
{
    mCancellable = g_cancellable_new();
}

FileInfo::FileInfo(const QString &uri, QObject *parent) : QObject(parent)
{
    CT_SYSLOG(LOG_DEBUG, "FileInfo construct ...");
    QUrl url(uri);

    mCancellable = g_cancellable_new();

    mUri = url.toDisplayString();
    mFile = g_file_new_for_uri(mUri.toUtf8().constData());
    mParent = g_file_get_parent(mFile);
    mIsRemote = !g_file_is_native(mFile);
    GFileType type = g_file_query_file_type(mFile, G_FILE_QUERY_INFO_NONE, nullptr);

    switch (type) {
    case G_FILE_TYPE_DIRECTORY:
        mIsDir = true;
        break;
    case G_FILE_TYPE_MOUNTABLE:
        mIsVolume = true;
        break;
    default:
        break;
    }
    CT_SYSLOG(LOG_DEBUG, "FileInfo construct ok");
}

FileInfo::~FileInfo()
{
    CT_SYSLOG(LOG_DEBUG, "FileInfo free ...");
    disconnect();

    g_object_unref(mCancellable);
    g_object_unref(mFile);

    if (mTargetFile) {
        g_object_unref(mTargetFile);
    }

    if (mParent) {
        g_object_unref(mParent);
    }

    mUri = nullptr;
    CT_SYSLOG(LOG_DEBUG, "FileInfo free ok!");
}

std::shared_ptr<FileInfo> FileInfo::fromUri(QString uri, bool addToHash)
{
    CT_SYSLOG(LOG_DEBUG, "start ...");
    FileInfoManager *infoManager = FileInfoManager::getInstance();
    infoManager->lock();
    std::shared_ptr<FileInfo> info = infoManager->findFileInfoByUri(uri);
    if (info != nullptr) {
        infoManager->unlock();
        return info;
    } else {
        std::shared_ptr<FileInfo> newlyInfo = std::make_shared<FileInfo>();
        QUrl url(uri);
        newlyInfo->mUri = url.toDisplayString();
        newlyInfo->mFile = g_file_new_for_uri(newlyInfo->mUri.toUtf8().constData());
        newlyInfo->mParent = g_file_get_parent(newlyInfo->mFile);
        newlyInfo->mIsRemote = !g_file_is_native(newlyInfo->mFile);
        GFileType type = g_file_query_file_type(newlyInfo->mFile, G_FILE_QUERY_INFO_NONE, nullptr);

        switch (type) {
        case G_FILE_TYPE_DIRECTORY:
            newlyInfo->mIsDir = true;
            break;
        case G_FILE_TYPE_MOUNTABLE:
            newlyInfo->mIsVolume = true;
            break;
        default:
            break;
        }

        if (addToHash) {
            newlyInfo = infoManager->insertFileInfo(newlyInfo);
        }

        infoManager->unlock();

        return newlyInfo;
    }
}

std::shared_ptr<FileInfo> FileInfo::fromPath(QString path, bool addToHash)
{
    QString uri = "file://" + path;

    return fromUri(uri, addToHash);
}

std::shared_ptr<FileInfo> FileInfo::fromGFile(GFile *file, bool addToHash)
{
    char *uriStr = g_file_get_uri(file);
    QString uri = uriStr;

    g_free(uriStr);

    return fromUri(uri, addToHash);
}

QString FileInfo::uri()
{
    return mUri;
}



bool FileInfo::isDir()
{
    return mIsDir || mContentType == "inode/directory";
}

bool FileInfo::isVolume()
{
    return mIsVolume;
}

bool FileInfo::isSymbolLink()
{
    return mIsSymbolLink;
}

bool FileInfo::isVirtual()
{
    return mIsVirtual;
}

QString FileInfo::displayName()
{
    return mDisplayName;
}

QString FileInfo::iconName()
{
    return mIconName;
}

QString FileInfo::symbolicIconName()
{
    return mSymbolicIconName;
}

quint64 FileInfo::accessTime()
{
    return mAccessTime;
}

bool FileInfo::canRead()
{
    return mCanRead;
}

bool FileInfo::canWrite()
{
    return mCanWrite;
}

bool FileInfo::canExecute()
{
    return mCanExcute;
}

bool FileInfo::canMount()
{
    return mCanMount;
}

bool FileInfo::canUnmount()
{
    return mCanUnmount;
}

bool FileInfo::canEject()
{
    return mCanEject;
}

bool FileInfo::canStart()
{
    return mCanStart;
}

bool FileInfo::canStop()
{
    return mCanStop;
}

bool FileInfo::isDesktopFile()
{
    return mCanExcute && mUri.endsWith(".desktop");
}

bool FileInfo::isEmptyInfo()
{
    return mDisplayName == nullptr;
}

FileInfo::AccessFlags FileInfo::accesses()
{
    auto flags = AccessFlags();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    flags.setFlag(Readable, mCanRead);
    flags.setFlag(Writeable, mCanWrite);
    flags.setFlag(Executable, mCanExcute);
    flags.setFlag(Deleteable, mCanDelete);
    flags.setFlag(Trashable, mCanTrash);
    flags.setFlag(Renameable, mCanRename);
    return flags;
#else
    flags = 0;
    if (m_can_read)
        flags |= Readable;
    if (m_can_write)
        flags |= Writeable;
    if (m_can_excute)
        flags |= Executable;
    if (m_can_delete)
        flags |= Deleteable;
    if (m_can_trash)
        flags |= Trashable;
    if (m_can_rename)
        flags |= Renameable;
#endif
}

GFile *FileInfo::gFileHandle()
{
    return mFile;
}

bool FileInfo::canRename()
{
    return mCanRename;
}

bool FileInfo::canDelete()
{
    return mCanDelete;
}

bool FileInfo::canTrash()
{
    return mCanTrash;
}

quint64 FileInfo::size()
{
    return mSize;
}

quint64 FileInfo::modifiedTime()
{
    return mModifiedTime;
}

QString FileInfo::accessDate()
{
    return mAccessDate;
}

QString FileInfo::type()
{
    return mContentType;
}

QString FileInfo::fileID()
{
    return mFileId;
}

QString FileInfo::mimeType()
{
    return mMimeTypeString;
}

QString FileInfo::fileType()
{
    return mFileType;
}

QString FileInfo::fileSize()
{
    return mFileSize;
}

QString FileInfo::modifiedDate()
{
    return mModifiedDate;
}
