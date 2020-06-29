#include "file-operation.h"
#include "file-operation-manager.h"

#include <QApplication>


FileOperation::FileOperation(QObject *parent) : QObject (parent)
{
    mCancellableWrapper = wrapGCancellable(g_cancellable_new());
    setAutoDelete(true);
}

FileOperation::~FileOperation()
{

}

void FileOperation::run()
{

}

void FileOperation::setHasError(bool hasError)
{
    mHasError = hasError;
}

bool FileOperation::hasError()
{
    return mHasError;
}

std::shared_ptr<FileOperationInfo> FileOperation::getOperationInfo()
{
    return nullptr;
}

void FileOperation::setShouldReversible(bool reversible)
{
    mReversible = reversible;
}

bool FileOperation::reversible()
{
    return mReversible;
}

bool FileOperation::isCancelled()
{
    return mIsCancelled;
}

void FileOperation::slotCancel()
{

}

GCancellableWrapperPtr FileOperation::getCancellable()
{
    return mCancellableWrapper;
}

void FileOperation::notifyFileWatcherOperationFinished()
{
    if (!qApp->allWidgets().isEmpty()) {
        auto info = this->getOperationInfo();
        if (info) {
            FileOperationManager::getInstance()->manuallyNotifyDirectoryChanged(info.get());
        }
    }
}
