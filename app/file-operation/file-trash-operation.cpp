#include "file-trash-operation.h"


FileTrashOperation::FileTrashOperation(QStringList srcUris, QObject *parent)
{

}

void FileTrashOperation::run()
{

}

std::shared_ptr<FileOperationInfo> FileTrashOperation::getOperationInfo()
{
    return mInfo;
}
