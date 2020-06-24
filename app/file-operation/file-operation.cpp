#include "file-operation.h"


FileOperation::FileOperation(QObject *parent)
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

GCancellableWrapperPtr FileOperation::getCancellable()
{
    return mCancellableWrapper;
}
