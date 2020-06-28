#include "file-node-reporter.h"

FileNodeReporter::FileNodeReporter(QObject *parent)
{

}

FileNodeReporter::~FileNodeReporter()
{

}

void FileNodeReporter::slotCancel()
{
    mCancelled = true;
}

bool FileNodeReporter::isOperationCancelled()
{
    return mCancelled;
}

void FileNodeReporter::sendNodeFound(const QString &uri, const qint64 &offset)
{
    Q_EMIT nodeFound(uri, offset);
}
