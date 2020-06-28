#include "file-link-operation.h"


FileLinkOperation::FileLinkOperation(QString srcUri, QString destDirUri, QObject *parent)
{

}

FileLinkOperation::~FileLinkOperation()
{

}

void FileLinkOperation::run()
{

}

std::shared_ptr<FileOperationInfo> FileLinkOperation::getOperationInfo()
{
    return mInfo;
}

