#include "file-rename-operation.h"


FileRenameOperation::FileRenameOperation(QString uri, QString newName)
{

}

void FileRenameOperation::run()
{

}

std::shared_ptr<FileOperationInfo> FileRenameOperation::getOperationInfo()
{
    return mInfo;
}
