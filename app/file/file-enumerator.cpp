#include "file-enumerator.h"

#include "file-info.h"
#include "mount-operation.h"

#include <QTimer>
#include <QMessageBox>
#include <file-utils.h>
#include <clib_syslog.h>

#ifndef FM_FIND_NEXT_FILES_BATCH_SIZE
#define FM_FIND_NEXT_FILES_BATCH_SIZE 100
#endif

FileEnumerator::FileEnumerator(QObject *parent) : QObject(parent)
{
    CT_SYSLOG(LOG_DEBUG, "construct start...");
    mRootFile = g_file_new_for_uri("file:///");
    mCancellable = g_cancellable_new();
    mChildrenUris = new QList<QString>();
    connect(this, &FileEnumerator::enumerateFinished, this, [=]() {
        CT_SYSLOG(LOG_DEBUG, "FileEnumerator::enumerateFinished");
        if (mAutoDelete) {
            this->deleteLater();
        }
    });
    CT_SYSLOG(LOG_DEBUG, "construct end!");
}

FileEnumerator::~FileEnumerator()
{
    CT_SYSLOG(LOG_DEBUG, "free start...");
    disconnect();
    g_object_unref(mRootFile);
    g_object_unref(mCancellable);

    delete mChildrenUris;
    CT_SYSLOG(LOG_DEBUG, "free end!");
}

void FileEnumerator::prepare()
{
    CT_SYSLOG(LOG_DEBUG, "prepare start...");
    GError *err = nullptr;
    GFileEnumerator *enumerator = g_file_enumerate_children(mRootFile, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, mCancellable, &err);
    if (err) {
        handleError(err);
        g_error_free(err);
    } else {
        g_object_unref(enumerator);
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
        QTimer::singleShot(100, this, [=]() {
#else
        QTimer::singleShot(100, [=]() {
#endif
            Q_EMIT prepared(nullptr);
        });
    }
    CT_SYSLOG(LOG_DEBUG, "prepare end!");
}

void FileEnumerator::enumerateSync()
{
    CT_SYSLOG(LOG_DEBUG, "enumerateSync start...");
    GFile *target = enumerateTargetFile();

    GFileEnumerator *enumerator = g_file_enumerate_children(target, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, mCancellable, nullptr);
    if (enumerator) {
        enumerateChildren(enumerator);

        g_file_enumerator_close_async(enumerator, 0, nullptr, nullptr, nullptr);
        g_object_unref(enumerator);
    } else {
        Q_EMIT this->enumerateFinished(false);
    }

    g_object_unref(target);
    CT_SYSLOG(LOG_DEBUG, "enumerateSync end!");
}

const QStringList FileEnumerator::getChildrenUris()
{

}

void FileEnumerator::setEnumerateDirectory(QString uri)
{
    CT_SYSLOG(LOG_DEBUG, "setEnumerateDirectory start...");
    if (mCancellable) {
        g_cancellable_cancel(mCancellable);
        g_object_unref(mCancellable);
    }
    mCancellable = g_cancellable_new();

    if (mRootFile) {
        g_object_unref(mRootFile);
    }

#if GLIB_CHECK_VERSION(2, 50, 0)
    mRootFile = g_file_new_for_uri(uri.toUtf8());
#else
    if (uri.startsWith("search:///"))
    {
        mRootFile = peony_search_vfs_file_new_for_uri(uri.toUtf8());
    }
    else
        mRootFile = g_file_new_for_uri(uri.toUtf8());
#endif
    CT_SYSLOG(LOG_DEBUG, "setEnumerateDirectory end!");
}

void FileEnumerator::setEnumerateDirectory(GFile *file)
{
    CT_SYSLOG(LOG_DEBUG, "setEnumerateDirectory start...");
    if (mCancellable) {
        g_cancellable_cancel(mCancellable);
        g_object_unref(mCancellable);
    }
    mCancellable = g_cancellable_new();

    if (mRootFile) {
        g_object_unref(mRootFile);
    }
    mRootFile = g_file_dup(file);
    CT_SYSLOG(LOG_DEBUG, "setEnumerateDirectory end!");
}

void FileEnumerator::setAutoDelete(bool autoDelete)
{
    Q_UNUSED(autoDelete)
}

const QList<std::shared_ptr<FileInfo> > FileEnumerator::getChildren(bool addToHash)
{
    CT_SYSLOG(LOG_DEBUG, "getChildren start...");
    QList<std::shared_ptr<FileInfo>> children;
    for (auto uri : *mChildrenUris) {
        auto file_info = FileInfo::fromUri(uri, addToHash);
        children<<file_info;
    }

    CT_SYSLOG(LOG_DEBUG, "getChildren end!");

    return children;
}

void FileEnumerator::slotCancel()
{
    CT_SYSLOG(LOG_DEBUG, "slotCancel start...");
    g_cancellable_cancel(mCancellable);
    g_object_unref(mCancellable);
    mCancellable = g_cancellable_new();

    mChildrenUris->clear();

    CT_SYSLOG(LOG_DEBUG, "slotCancel end!");

    Q_EMIT enumerateFinished(false);
}

void FileEnumerator::slotEnumerateAsync()
{
    CT_SYSLOG(LOG_DEBUG, "slotEnumerateAsync start...");
    g_file_enumerate_children_async(mRootFile, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT, mCancellable, GAsyncReadyCallback(findChildrenAsyncReadyCallback), this);

    CT_SYSLOG(LOG_DEBUG, "slotEnumerateAsync end!");
}

GFile *FileEnumerator::enumerateTargetFile()
{
    CT_SYSLOG(LOG_DEBUG, "enumerateTargetFile start...");
    GFileInfo *info = g_file_query_info(mRootFile, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, G_FILE_QUERY_INFO_NONE, nullptr, nullptr);
    char *uri = nullptr;
    uri = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);
    g_object_unref(info);

    GFile *target = nullptr;
    if (uri) {
        target = g_file_new_for_uri(uri);
        g_free(uri);
    } else {
        target = g_file_dup(mRootFile);
    }

    CT_SYSLOG(LOG_DEBUG, "enumerateTargetFile end!");

    return target;
}

void FileEnumerator::handleError(GError *err)
{
    CT_SYSLOG(LOG_DEBUG, "handleError start ...");
    switch (err->code) {
    case G_IO_ERROR_NOT_DIRECTORY: {
        auto uri = g_file_get_uri(mRootFile);
        auto targetUri = FileUtils::getTargetUri(uri);
        if (uri) {
            g_free(uri);
        }
        if (!targetUri.isEmpty()) {
            prepared(nullptr, targetUri);
            return;
        }

        bool isMountable = false;
        GFileInfo *fileMountInfo = g_file_query_info(mRootFile, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, nullptr, nullptr);
        if (fileMountInfo) {
            isMountable = g_file_info_get_attribute_boolean(fileMountInfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT);
            g_object_unref(fileMountInfo);
        }

        if (isMountable) {
            g_file_mount_mountable(mRootFile, G_MOUNT_MOUNT_NONE, nullptr, mCancellable, GAsyncReadyCallback(mountMountableCallback), this);
        } else {
            g_file_mount_enclosing_volume(mRootFile, G_MOUNT_MOUNT_NONE, nullptr, mCancellable, GAsyncReadyCallback(mountEnclosingVolumeCallback), this);
        }
        break;
    }
    case G_IO_ERROR_NOT_MOUNTED:
        g_file_mount_enclosing_volume(mRootFile, G_MOUNT_MOUNT_NONE, nullptr, mCancellable, GAsyncReadyCallback(mountEnclosingVolumeCallback), this);
        break;
    case G_IO_ERROR_NOT_SUPPORTED:
        QMessageBox::critical(nullptr, tr("Error"), err->message);
        break;
    case G_IO_ERROR_PERMISSION_DENIED:
        QMessageBox::critical(nullptr, tr("Error"), err->message);
        break;
    case G_IO_ERROR_NOT_FOUND:
        Q_EMIT prepared(GerrorWrapper::wrapFrom(g_error_new(G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "file not found")));
        break;
    default:
        Q_EMIT prepared(GerrorWrapper::wrapFrom(err), nullptr, true);
        break;
    }
    CT_SYSLOG(LOG_DEBUG, "handleError end!");
}

void FileEnumerator::enumerateChildren(GFileEnumerator *enumerator)
{
    CT_SYSLOG(LOG_DEBUG, "enumerateChildren start ...");
    GFileInfo *info = nullptr;
    GFile *child = nullptr;
    info = g_file_enumerator_next_file(enumerator, mCancellable, nullptr);
    if (!info) {
        Q_EMIT enumerateFinished(false);
        return;
    }
    while (info) {
        child = g_file_enumerator_get_child(enumerator, info);
        char *uri = g_file_get_uri(child);
        char *path = g_file_get_path(child);
        g_object_unref(child);
        if (path) {
            QString localUri = QString("file://%1").arg(path);
            *mChildrenUris << localUri;
            g_free(path);
        } else {
            *mChildrenUris << uri;
        }

        g_free(uri);
        g_object_unref(info);
        info = g_file_enumerator_next_file(enumerator, mCancellable, nullptr);
    }
    Q_EMIT enumerateFinished(true);
    CT_SYSLOG(LOG_DEBUG, "enumerateChildren end!");
}

GAsyncReadyCallback FileEnumerator::mountMountableCallback(GFile *file, GAsyncResult *res, FileEnumerator *pThis)
{
    CT_SYSLOG(LOG_DEBUG, "mountMountableCallback start ...");
    GError *err = nullptr;
    GFile *target = g_file_mount_mountable_finish(file, res, &err);
    if (err && err->code != 0) {
        auto err_data = GerrorWrapper::wrapFrom(err);
        Q_EMIT pThis->prepared(err_data);
    } else {
        Q_EMIT pThis->prepared(nullptr);
    }
    if (target) {
        g_object_unref(target);
    }


    CT_SYSLOG(LOG_DEBUG, "mountMountableCallback end!");
    return nullptr;
}

GAsyncReadyCallback FileEnumerator::mountEnclosingVolumeCallback(GFile *file, GAsyncResult *res, FileEnumerator *pThis)
{
    CT_SYSLOG(LOG_DEBUG, "mountEnclosingVolumeCallback start ...");
    GError *err = nullptr;
    if (g_file_mount_enclosing_volume_finish (file, res, &err)) {
        if (err) {
            Q_EMIT pThis->prepared(GerrorWrapper::wrapFrom(err), nullptr, true);
        } else {
            Q_EMIT pThis->prepared();
        }
    } else {
        if (err) {
            if (err->code == G_IO_ERROR_ALREADY_MOUNTED) {
                Q_EMIT pThis->prepared(GerrorWrapper::wrapFrom(err));
                return nullptr;
            }
            if (!pThis->mRootFile) {
                return nullptr;
            }

            char *uri = g_file_get_uri(file);
            MountOperation *op = new MountOperation(uri);
            op->setAutoDelete();
            g_free(uri);
            //op->setAutoDelete();
            pThis->connect(op, &MountOperation::cancelled, pThis, [pThis]() {
                Q_EMIT pThis->enumerateFinished(false);
            });

            pThis->connect(op, &MountOperation::mountSuccess, pThis, [=] (QString uri) {
                Q_EMIT pThis->mountSuccess(uri);
            });

            pThis->connect(op, &MountOperation::finished, pThis, [=](const std::shared_ptr<GerrorWrapper> &finished_err) {
                if (finished_err) {
                    if (finished_err->code() == G_IO_ERROR_PERMISSION_DENIED) {
                        pThis->enumerateFinished(false);
                        QMessageBox::critical(nullptr, tr("Error"), finished_err->message());
                        return;
                    }
                    Q_EMIT pThis->prepared(finished_err);
                } else {
                    Q_EMIT pThis->prepared(nullptr);
                }
            });
            op->slotStart();
        }
    }

    CT_SYSLOG(LOG_DEBUG, "mountEnclosingVolumeCallback end!!");

    return nullptr;
}

GAsyncReadyCallback FileEnumerator::findChildrenAsyncReadyCallback(GFile *file, GAsyncResult *res, FileEnumerator *pThis)
{
    CT_SYSLOG(LOG_DEBUG, "findChildrenAsyncReadyCallback start ...");
    GError *err = nullptr;
    GFileEnumerator *enumerator = g_file_enumerate_children_finish(file, res, &err);
    if (err) {
        if (err->code == G_IO_ERROR_NOT_MOUNTED) {
            g_object_unref(pThis->mRootFile);
            pThis->mRootFile = g_file_dup(file);
            pThis->prepare();
            return nullptr;
        }
        g_error_free(err);
    }

    g_file_enumerator_next_files_async(enumerator, FM_FIND_NEXT_FILES_BATCH_SIZE, G_PRIORITY_DEFAULT, pThis->mCancellable, GAsyncReadyCallback(enumeratorNextFilesAsyncReadyCallback), pThis);

    g_object_unref(enumerator);

    CT_SYSLOG(LOG_DEBUG, "findChildrenAsyncReadyCallback end!");

    return nullptr;
}

GAsyncReadyCallback FileEnumerator::enumeratorNextFilesAsyncReadyCallback(GFileEnumerator *enumerator, GAsyncResult *res, FileEnumerator *pThis)
{
    CT_SYSLOG(LOG_DEBUG, "enumeratorNextFilesAsyncReadyCallback start ...");
    if ((nullptr == enumerator) || (nullptr == res)) {
        CT_SYSLOG(LOG_WARNING, "input param is nullptr!");
        return nullptr;
    }

    GError *err = nullptr;
    GList *files = g_file_enumerator_next_files_finish(enumerator, res, &err);
    if (!files && !err) {
        Q_EMIT pThis->enumerateFinished(true);
        CT_SYSLOG(LOG_WARNING, "findChildrenAsyncReadyCallback error end!");
        return nullptr;
    }

    if (!files && err) {
        CT_SYSLOG(LOG_DEBUG, "findChildrenAsyncReadyCallback end!");
        return nullptr;
    }
    if (err) {
        CT_SYSLOG(LOG_DEBUG, "next files async, code:%d, msg:%s", err->code, err->message);
    }

    GList *l = files;
    QStringList uriList;
    int filesCount = 0;
    while (nullptr != l) {
        char* uri = nullptr;
        char* path = nullptr;
        GFileInfo *info = static_cast<GFileInfo*>(l->data);
        GFile *file = g_file_enumerator_get_child(enumerator, info);
        uri = g_file_get_uri(file);
        CT_SYSLOG(LOG_DEBUG, "file:%s", uri);
        path = g_file_get_path(file);
        g_object_unref(file);
        if (path) {
            QString localUri = QString("file://%1").arg(path);
            uriList << localUri;
            *(pThis->mChildrenUris) << localUri;
            g_free(path);
        } else {
            uriList << uri;
            *(pThis->mChildrenUris) << uri;
        }

        g_free(uri);
        ++ filesCount;
        l = l->next;
    }

    g_list_free_full(files, g_object_unref);

    Q_EMIT pThis->childrenUpdated(uriList);

    if (filesCount == FM_FIND_NEXT_FILES_BATCH_SIZE) {
        g_file_enumerator_next_files_async(enumerator, FM_FIND_NEXT_FILES_BATCH_SIZE, G_PRIORITY_DEFAULT, pThis->mCancellable, GAsyncReadyCallback(enumeratorNextFilesAsyncReadyCallback), pThis);
    } else {
        Q_EMIT pThis->enumerateFinished(true);
    }

    CT_SYSLOG(LOG_DEBUG, "findChildrenAsyncReadyCallback end!");

    return nullptr;
}

