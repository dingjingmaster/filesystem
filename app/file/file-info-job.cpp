#include "file-info-job.h"

FileInfoJob::FileInfoJob(std::shared_ptr<FileInfo> info, QObject *parent)
{

}

FileInfoJob::FileInfoJob(const QString &uri, QObject *parent)
{

}

FileInfoJob::~FileInfoJob()
{

}

bool FileInfoJob::querySync()
{

}

std::shared_ptr<FileInfo> FileInfoJob::getInfo()
{
    return mInfo;
}

void FileInfoJob::setAutoDelete(bool deleteWhenJobFinished)
{
    mAutoDelete = deleteWhenJobFinished;
}

void FileInfoJob::cancel()
{

}

void FileInfoJob::queryAsync()
{

}

GAsyncReadyCallback FileInfoJob::queryInfoAsyncCallback(GFile *file, GAsyncResult *res, FileInfoJob *thisJob)
{

}

void FileInfoJob::refreshInfoContents(GFileInfo *newInfo)
{

}
