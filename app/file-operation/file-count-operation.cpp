#include "file-count-operation.h"


FileCountOperation::FileCountOperation(const QStringList &uris, bool countRoot, QObject *parent)
{

}

FileCountOperation::~FileCountOperation()
{

}

void FileCountOperation::run()
{

}

std::shared_ptr<FileOperationInfo> FileCountOperation::getOperationInfo()
{
    return nullptr;
}

void FileCountOperation::getInfo(quint64 &file_count, quint64 &hidden_file_count, quint64 &total_size)
{
    file_count = mFileCount;
    hidden_file_count = mHiddenFileCount;
    total_size = mTotalSize;
}
