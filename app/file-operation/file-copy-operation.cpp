#include "file-copy-operation.h"

FileCopyOperation::FileCopyOperation(QStringList sourceUris, QString destDirUri, QObject *parent)
{

}

FileCopyOperation::~FileCopyOperation()
{

}

void FileCopyOperation::run()
{

}

std::shared_ptr<FileOperationInfo> FileCopyOperation::getOperationInfo()
{
    return mInfo;
}

void FileCopyOperation::slotCancel()
{

}

FileOperation::ResponseType FileCopyOperation::prehandle(GError *err)
{

}

void FileCopyOperation::copyRecursively(FileNode *node)
{

}

void FileCopyOperation::rollbackNodeRecursively(FileNode *node)
{

}

void FileCopyOperation::progressCallback(goffset currentNumBytes, goffset totalNumBytes, FileCopyOperation *pThis)
{

}
