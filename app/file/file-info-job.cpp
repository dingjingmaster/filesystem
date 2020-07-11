#include "file-info-job.h"

#include "file-info.h"
#include "clib_syslog.h"
#include "file-meta-info.h"
#include "file-info-manager.h"

#include <QUrl>
#include <QIcon>
#include <QTime>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

FileInfoJob::FileInfoJob(std::shared_ptr<FileInfo> info, QObject *parent) : QObject(parent)
{
    mInfo = info;
    connect(mInfo.get(), &FileInfo::updated, this, &FileInfoJob::infoUpdated);
}

FileInfoJob::FileInfoJob(const QString &uri, QObject *parent) : QObject(parent)
{
    auto info = FileInfo::fromUri(uri);
    mInfo = info;
    connect(mInfo.get(), &FileInfo::updated, this, &FileInfoJob::infoUpdated);
}

FileInfoJob::~FileInfoJob()
{
    if (mInfo.use_count() <= 2) {
        FileInfoManager *mgr = FileInfoManager::getInstance();
        mgr->remove(mInfo);
    }
}

bool FileInfoJob::querySync()
{
    FileInfo *info = nullptr;
    if (auto data = mInfo.get()) {
        info = data;
    } else {
        if (mAutoDelete) {
            deleteLater();
        }
        return false;
    }
    GError *err = nullptr;

    auto _info = g_file_query_info(info->mFile, "standard::*," "time::*," "access::*," "mountable::*," "metadata::*," G_FILE_ATTRIBUTE_ID_FILE, G_FILE_QUERY_INFO_NONE, nullptr, &err);
    if (err) {
        CT_SYSLOG(LOG_WARNING, "error code:%d, msg:%s", err->code, err->message);
        g_error_free(err);
        if (mAutoDelete) {
            deleteLater();
        }
        return false;
    }

    refreshInfoContents(_info);
    g_object_unref(_info);
    if (mAutoDelete) {
        deleteLater();
    }
    return true;
}

std::shared_ptr<FileInfo> FileInfoJob::getInfo()
{
    return mInfo;
}

void FileInfoJob::setAutoDelete(bool deleteWhenJobFinished)
{
    mAutoDelete = deleteWhenJobFinished;
}

void FileInfoJob::cancel()
{
    g_cancellable_cancel(mInfo->mCancellable);
    g_object_unref(mInfo->mCancellable);
    mInfo->mCancellable = g_cancellable_new();
}

void FileInfoJob::queryAsync()
{
    FileInfo *info = nullptr;
    if (auto data = mInfo) {
        info = data.get();
        cancel();
    } else {
        Q_EMIT queryAsyncFinished(false);
        return;
    }
    g_file_query_info_async(info->mFile, "standard::*," "time::*," "access::*," "mountable::*," "metadata::*," G_FILE_ATTRIBUTE_ID_FILE, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT, info->mCancellable, GAsyncReadyCallback(queryInfoAsyncCallback), this);
    if (mAutoDelete) {
        connect(this, &FileInfoJob::queryAsyncFinished, this, &FileInfoJob::deleteLater, Qt::QueuedConnection);
    }
}

QString FileInfoJob::getAppName(QString desktopfp)
{
    GError** error = nullptr;
    GKeyFileFlags flags = G_KEY_FILE_NONE;
    GKeyFile* keyfile = g_key_file_new ();

    QByteArray fpbyte = desktopfp.toLocal8Bit();
    char* filepath = fpbyte.data();
    g_key_file_load_from_file(keyfile, filepath, flags, error);

    char* name = g_key_file_get_locale_string(keyfile, "Desktop Entry", "Name", nullptr, nullptr);
    QString namestr = QString::fromLocal8Bit(name);

    g_key_file_free(keyfile);
    return namestr;
}

GAsyncReadyCallback FileInfoJob::queryInfoAsyncCallback(GFile *file, GAsyncResult *res, FileInfoJob *thisJob)
{
    GError *err = nullptr;

    GFileInfo *_info = g_file_query_info_finish(file, res, &err);
    if (_info != nullptr) {
        thisJob->refreshInfoContents(_info);
        g_object_unref(_info);
        Q_EMIT thisJob->queryAsyncFinished(true);
    }
    else {
        CT_SYSLOG(LOG_WARNING, "error code:%d, msg:%s", err->code, err->message);
        g_error_free(err);
        Q_EMIT thisJob->queryAsyncFinished(false);
        return nullptr;
    }

    return nullptr;
}

void FileInfoJob::refreshInfoContents(GFileInfo *newInfo)
{
    if (!mInfo->mMutex.tryLock(300)) {
        return;
    }

    FileInfo *info = nullptr;
    if (auto data = mInfo) {
        info = data.get();
    } else {
        return;
    }
    GFileType type = g_file_info_get_file_type (newInfo);
    switch (type) {
    case G_FILE_TYPE_DIRECTORY:
        info->mIsDir = true;
        break;
    case G_FILE_TYPE_MOUNTABLE:
        info->mIsVolume = true;
        break;
    default:
        break;
    }

    info->mIsSymbolLink = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK);
    if (g_file_info_has_attribute(newInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_READ)) {
        info->mCanRead = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
    } else {
        // we assume an unknow access file is readable.
        info->mCanRead = true;
    }
    info->mCanWrite = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);
    info->mCanExcute = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE);
    info->mCanDelete = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE);
    info->mCanTrash = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH);
    info->mCanRename = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME);

    info->mCanMount = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT);
    info->mCanUnmount = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_UNMOUNT);
    info->mCanEject = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_EJECT);
    info->mCanStart = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START);
    info->mCanStop = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_STOP);

    info->mIsVirtual = g_file_info_get_attribute_boolean(newInfo, G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL);

    info->mDisplayName = QString (g_file_info_get_display_name(newInfo));
    GIcon *g_icon = g_file_info_get_icon (newInfo);
    if (G_IS_ICON(g_icon)) {
        const gchar* const* icon_names = g_themed_icon_get_names(G_THEMED_ICON (g_icon));
        if (icon_names) {
            auto p = icon_names;
            while (*p) {
                QIcon icon = QIcon::fromTheme(*p);
                if (!icon.isNull()) {
                    info->mIconName = QString (*p);
                    break;
                } else {
                    p++;
                }
            }
        }
        //g_object_unref(g_icon);
    }

    GIcon *g_symbolic_icon = g_file_info_get_symbolic_icon (newInfo);
    if (G_IS_ICON(g_symbolic_icon)) {
        const gchar* const* symbolic_icon_names = g_themed_icon_get_names(G_THEMED_ICON (g_symbolic_icon));
        if (symbolic_icon_names)
            info->mSymbolicIconName = QString (*symbolic_icon_names);
        //g_object_unref(g_symbolic_icon);
    }

    info->mFileId = g_file_info_get_attribute_string(newInfo, G_FILE_ATTRIBUTE_ID_FILE);

    info->mContentType = g_file_info_get_content_type (newInfo);
    if (info->mContentType == nullptr) {
        if (g_file_info_has_attribute(newInfo, "standard::fast-content-type")) {
            info->mContentType = g_file_info_get_attribute_string(newInfo, "standard::fast-content-type");
        }
    }

    info->mSize = g_file_info_get_attribute_uint64(newInfo, G_FILE_ATTRIBUTE_STANDARD_SIZE);
    info->mModifiedTime = g_file_info_get_attribute_uint64(newInfo, G_FILE_ATTRIBUTE_TIME_MODIFIED);
    info->mAccessTime = g_file_info_get_attribute_uint64(newInfo, G_FILE_ATTRIBUTE_TIME_ACCESS);

    info->mMimeTypeString = info->mContentType;
    if (!info->mMimeTypeString.isEmpty()) {
        char *content_type = g_content_type_get_description (info->mMimeTypeString.toUtf8().constData());
        info->mFileType = content_type;
        g_free (content_type);
        content_type = nullptr;
    }

    char *size_full = g_format_size_full(info->mSize, G_FORMAT_SIZE_DEFAULT);
    info->mFileSize = size_full;
    g_free(size_full);

    QDateTime date = QDateTime::fromMSecsSinceEpoch(info->mModifiedTime*1000);
    info->mModifiedDate = date.toString(Qt::SystemLocaleShortDate);

    date = QDateTime::fromMSecsSinceEpoch(info->mAccessTime*1000);
    info->mAccessDate = date.toString(Qt::SystemLocaleShortDate);

    mInfo->mMetaInfo = FileMetaInfo::fromGFileInfo(mInfo->uri(), newInfo);

    if (info->isDesktopFile()) {
        QUrl url = info->uri();
        GDesktopAppInfo *desktop_info = g_desktop_app_info_new_from_filename(url.path().toUtf8());
        if (!desktop_info) {
            mInfo->mMutex.unlock();
            info->updated();
            return;
        }
#if GLIB_CHECK_VERSION(2, 56, 0)
        auto str = g_desktop_app_info_get_locale_string(desktop_info, "Name");
#else
        //FIXME: should handle locale?
        //change "Name" to QLocale::system().name(),
        //try to fix Qt5.6 untranslated desktop file issue
        auto key = "Name[" +  QLocale::system().name() + "]";
        auto string = g_desktop_app_info_get_string(desktop_info, key.toUtf8().constData());
#endif
        CT_SYSLOG(LOG_DEBUG, "get name string: %s; uri:%s; display name:%s", str, info->uri().toUtf8().constData(), info->displayName().toUtf8().constData());

        if (str) {
            info->mDisplayName = str;
            g_free(str);
        } else {
            QString path = "/usr/share/applications/" + info->displayName();
            auto name = getAppName(path);
            if (name.length() > 0) {
                info->mDisplayName = name;
            } else {
                str = g_desktop_app_info_get_string(desktop_info, "Name");
                if (str) {
                    info->mDisplayName = str;
                    g_free(str);
                }
            }
        }
        g_object_unref(desktop_info);
    }

    Q_EMIT info->updated();
    mInfo->mMutex.unlock();
}
