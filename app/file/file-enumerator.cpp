#include "file-enumerator.h"

#ifndef FM_FIND_NEXT_FILES_BATCH_SIZE
#define FM_FIND_NEXT_FILES_BATCH_SIZE 100
#endif

FileEnumerator::FileEnumerator(QObject *parent) : QObject(parent)
{
    mRootFile = g_file_new_for_uri("file:///");
    mCancellable = g_cancellable_new();

    mChildrenUris = new QList<QString>();

    connect(this, &FileEnumerator::enumerateFinished, this, [=]() {
        if (mAutoDelete) {
            this->deleteLater();
        }
    });
}

FileEnumerator::~FileEnumerator()
{
    disconnect();
    g_object_unref(mRootFile);
    g_object_unref(mCancellable);

    delete mChildrenUris;
}

void FileEnumerator::prepare()
{

}

void FileEnumerator::enumerateSync()
{

}

const QStringList FileEnumerator::getChildrenUris()
{

}

void FileEnumerator::setEnumerateDirectory(QString uri)
{

}

void FileEnumerator::setEnumerateDirectory(GFile *file)
{

}

void FileEnumerator::setAutoDelete(bool autoDelete)
{

}

const QList<std::shared_ptr<FileInfo> > FileEnumerator::getChildren(bool addToHash)
{

}

void FileEnumerator::slotCancel()
{

}

void FileEnumerator::slotEnumerateAsync()
{

}

GFile *FileEnumerator::enumerateTargetFile()
{

}

void FileEnumerator::handleError(GError *err)
{

}

void FileEnumerator::enumerateChildren(GFileEnumerator *enumerator)
{

}

GAsyncReadyCallback FileEnumerator::mountMountableCallback(GFile *file, GAsyncResult *res, FileEnumerator *pThis)
{

}

GAsyncReadyCallback FileEnumerator::mountEnclosingVolume_callback(GFile *file, GAsyncResult *res, FileEnumerator *pThis)
{

}

GAsyncReadyCallback FileEnumerator::findChildrenAsyncReadyCallback(GFile *file, GAsyncResult *res, FileEnumerator *pThis)
{

}

GAsyncReadyCallback FileEnumerator::enumeratorNextFilesAsyncReadyCallback(GFileEnumerator *enumerator, GAsyncResult *res, FileEnumerator *pThis)
{

}

