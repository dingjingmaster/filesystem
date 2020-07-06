#include "file-operation-manager.h"
#include "file-watcher.h"

#include <QUrl>
#include <file-utils.h>

#include <clib_syslog.h>
#include <model/filelabel-model.h>

FileWatcher::FileWatcher(QString uri, QObject *parent) : QObject(parent)
{
    CT_SYSLOG(LOG_DEBUG, "FileWatcher construct ...");
    mUri = uri;
    mTargetUri = uri;
    mFile = g_file_new_for_uri(uri.toUtf8().constData());
    mCancellable = g_cancellable_new();

    connect(FileLabelModel::getGlobalModel(), &FileLabelModel::fileLabelChanged, this, [=](const QString &uri) {
        auto parentUri = FileUtils::getParentUri(uri);
        if (parentUri == mUri || parentUri == mTargetUri) {
            Q_EMIT fileChanged(uri);
            CT_SYSLOG(LOG_DEBUG, "file '%s' label changed", uri.toUtf8().constData());
        }
    });

    //monitor target file if existed.
    CT_SYSLOG(LOG_DEBUG, "begin prepare ...");
    prepare();
    CT_SYSLOG(LOG_DEBUG, "prepare ok!");

    GError *err1 = nullptr;
    mMonitor = g_file_monitor_file(mFile, G_FILE_MONITOR_WATCH_MOVES, mCancellable, &err1);
    if (err1) {
        g_error_free(err1);
        mSupportMonitor = false;
    }

    GError *err2 = nullptr;
    mDirMonitor = g_file_monitor_directory(mFile, G_FILE_MONITOR_NONE, mCancellable, &err2);
    if (err2) {
        g_error_free(err2);
        mSupportMonitor = false;
    }

    FileOperationManager::getInstance()->registerFileWatcher(this);

    CT_SYSLOG(LOG_DEBUG, "FileWatcher construct successful!");
}

FileWatcher::~FileWatcher()
{
    FileOperationManager::getInstance()->unregisterFileWatcher(this);
    disconnect();
    stopMonitor();
    slotCancel();

    g_object_unref(mCancellable);
    g_object_unref(mDirMonitor);
    g_object_unref(mMonitor);
    g_object_unref(mFile);
}

void FileWatcher::stopMonitor()
{
    GFileInfo *info = g_file_query_info(mFile, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, G_FILE_QUERY_INFO_NONE, mCancellable, nullptr);

    char *uri = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);

    if (uri) {
        g_object_unref(mFile);
        mFile = g_file_new_for_uri(uri);
        mTargetUri = uri;
        g_free(uri);
    }

    g_object_unref(info);
}

void FileWatcher::startMonitor()
{
    stopMonitor();
    mFileHandle = g_signal_connect(mMonitor, "changed", G_CALLBACK(file_changed_callback), this);
    mDirHandle = g_signal_connect(mDirMonitor, "changed", G_CALLBACK(dir_changed_callback), this);
}

bool FileWatcher::supportMonitor()
{
    return mSupportMonitor;
}

const QString FileWatcher::currentUri()
{
    return mUri;
}

void FileWatcher::setMonitorChildrenChange(bool monitorChildrenChange)
{
    mMontorChildrenChange = monitorChildrenChange;
}

void FileWatcher::slotCancel()
{
    g_cancellable_cancel(mCancellable);
    g_object_unref(mCancellable);
    mCancellable = g_cancellable_new();
}

void FileWatcher::prepare()
{
    GFileInfo *info = g_file_query_info(mFile, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, G_FILE_QUERY_INFO_NONE, mCancellable, nullptr);

    char *uri = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);

    if (uri) {
        g_object_unref(mFile);
        mFile = g_file_new_for_uri(uri);
        mTargetUri = uri;
        g_free(uri);
    }

    g_object_unref(info);
}

void FileWatcher::changeMonitorUri(QString uri)
{
    QString oldUri = mUri;

    stopMonitor();
    slotCancel();

    mUri = uri;
    mTargetUri = uri;
    g_object_unref(mFile);
    g_object_unref(mMonitor);
    g_object_unref(mDirMonitor);

    mFile = g_file_new_for_uri(uri.toUtf8().constData());

    prepare();

    GError *err1 = nullptr;
    mMonitor = g_file_monitor_file(mFile, G_FILE_MONITOR_WATCH_MOVES, mCancellable, &err1);
    if (err1) {
        mSupportMonitor = false;
        g_error_free(err1);
    }

    GError *err2 = nullptr;
    mDirMonitor = g_file_monitor_directory(mFile, G_FILE_MONITOR_NONE, mCancellable, &err2);
    if (err2) {
        mSupportMonitor = false;
        g_error_free(err2);
    }

    startMonitor();

    Q_EMIT locationChanged(oldUri, mUri);
}

void FileWatcher::dir_changed_callback(GFileMonitor *monitor, GFile *file, GFile *otherFile, GFileMonitorEvent eventType, FileWatcher *pThis)
{
    Q_UNUSED(monitor);
    Q_UNUSED(otherFile);
    switch (eventType) {
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGED: {
        if (pThis->mMontorChildrenChange) {
            char *uri = g_file_get_uri(file);
            QString changedFileUri = uri;
            QUrl url = changedFileUri;
            changedFileUri = url.toDisplayString();
            g_free(uri);
            Q_EMIT pThis->fileChanged(changedFileUri);
        }
        break;
    }
    case G_FILE_MONITOR_EVENT_CREATED: {
        char *uri = g_file_get_uri(file);
        QString createdFileUri = uri;
        QUrl url = createdFileUri;
        createdFileUri = url.toDisplayString();
        g_free(uri);
        Q_EMIT pThis->fileCreated(createdFileUri);
        break;
    }
    case G_FILE_MONITOR_EVENT_DELETED: {
        char *uri = g_file_get_uri(file);
        QString deletedFileUri = uri;
        QUrl url = deletedFileUri;
        deletedFileUri = url.toDisplayString();
        g_free(uri);
        Q_EMIT pThis->fileDeleted(deletedFileUri);
        break;
    }
    case G_FILE_MONITOR_EVENT_UNMOUNTED: {
        char *uri = g_file_get_uri(file);
        QString deletedFileUri = uri;
        QUrl url = deletedFileUri;
        deletedFileUri = url.toDisplayString();
        g_free(uri);
        Q_EMIT pThis->directoryUnmounted(deletedFileUri);
        break;
    }
    default:
        break;
    }
}

void FileWatcher::file_changed_callback(GFileMonitor *monitor, GFile *file, GFile *otherFile, GFileMonitorEvent eventType, FileWatcher *pThis)
{
    Q_UNUSED(monitor);
    Q_UNUSED(file);
    switch (eventType) {
    case G_FILE_MONITOR_EVENT_MOVED_IN:
    case G_FILE_MONITOR_EVENT_MOVED_OUT:
    case G_FILE_MONITOR_EVENT_RENAMED: {
        char *new_uri = g_file_get_uri(otherFile);
        QString uri = new_uri;
        QUrl url =  uri;
        uri = url.toDisplayString();
        g_free(new_uri);
        pThis->changeMonitorUri(uri);
        break;
    }
    case G_FILE_MONITOR_EVENT_DELETED: {
        pThis->stopMonitor();
        pThis->slotCancel();
        Q_EMIT pThis->directoryDeleted(pThis->mTargetUri);
        break;
    }
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED: {
        char *uri = g_file_get_uri(file);
        Q_EMIT pThis->fileChanged(uri);
        g_free(uri);
        break;
    }
    default:
        break;
    }
}
