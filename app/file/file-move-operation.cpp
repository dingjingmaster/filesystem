#include "file-move-operation.h"


FileMoveOperation::FileMoveOperation(QStringList sourceUris, QString destDirUri, QObject *parent)
{

}

FileMoveOperation::~FileMoveOperation()
{

}

void FileMoveOperation::run()
{

}

void FileMoveOperation::setCopyMove(bool copyMove)
{
    mCopyMove = copyMove;
}

void FileMoveOperation::setForceUseFallback(bool useFallback)
{
    mForceUseFallback = useFallback;
}

std::shared_ptr<FileOperationInfo> FileMoveOperation::getOperationInfo()
{
    return mInfo;
}

void FileMoveOperation::slotCancel()
{

}

void FileMoveOperation::move()
{

}

bool FileMoveOperation::isValid()
{

}

void FileMoveOperation::moveForceUseFallback()
{

}

FileOperation::ResponseType FileMoveOperation::prehandle(GError *err)
{

}

void FileMoveOperation::copyRecursively(FileNode *node)
{

}

void FileMoveOperation::deleteRecursively(FileNode *node)
{

}

void FileMoveOperation::progressCallback(goffset currentNumBytes, goffset totalNumBytes, FileMoveOperation *pThis)
{

}

void FileMoveOperation::rollbackNodeRecursively(FileNode *node)
{

}
