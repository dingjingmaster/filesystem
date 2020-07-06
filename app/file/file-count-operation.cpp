#include "file-count-operation.h"
#include "file-node-reporter.h"
#include "file-node.h"


FileCountOperation::FileCountOperation(const QStringList &uris, bool countRoot, QObject *parent) : FileOperation(parent)
{
    mCountRoot = countRoot;
    mReporter = new FileNodeReporter(this);
    //connect(m_reporter, &FileNodeReporter::nodeFound, this, &FileOperation::operationPreparedOne);
    connect(mReporter, &FileNodeReporter::nodeFound, [=](const QString &uri, quint64 size) {
        mFileCount++;
        if (uri.contains("/.")) {
            mHiddenFileCount++;
        }
        mTotalSize += size;
        Q_EMIT this->operationPreparedOne(uri, size);
    });
    mUris = uris;
}

FileCountOperation::~FileCountOperation()
{

}

void FileCountOperation::run()
{
    Q_EMIT operationStarted();
    if (mUris.isEmpty()) {
        Q_EMIT operationFinished();
    }

    QList<FileNode *> nodes;
    for (auto uri : mUris) {
        auto node = new FileNode(uri, nullptr, mReporter);
        node->findChildrenRecursively();
        nodes<<node;
    }
    if (!this->isCancelled()) {
        if (!mCountRoot) {
            for (auto node : nodes) {
                -- mFileCount;
                if (node->baseName().startsWith(".")) {
                    -- mHiddenFileCount;
                }
                //m_total_size -= node->size();
            }
        }
        Q_EMIT countDone(mFileCount, mHiddenFileCount, mTotalSize);
    }
    Q_EMIT operationPrepared();
    Q_EMIT operationFinished();
    for (auto node : nodes) {
        delete node;
    }
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

void FileCountOperation::slotCancel()
{
    FileOperation::slotCancel();
    mReporter->slotCancel();
}
