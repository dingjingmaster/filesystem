#include "file-untrash-operation.h"

FileUntrashOperation::FileUntrashOperation(QStringList uris, QObject *parent)
{

}

void FileUntrashOperation::run()
{

}

std::shared_ptr<FileOperationInfo> FileUntrashOperation::getOperationInfo()
{
    return mInfo;
}

void FileUntrashOperation::cacheOriginalUri()
{

}

const QString FileUntrashOperation::handleDuplicate(const QString &uri)
{

}
