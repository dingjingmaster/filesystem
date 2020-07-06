#include "file-delete-operation.h"
#include "file-node-reporter.h"
#include "file-node.h"


FileDeleteOperation::FileDeleteOperation(QStringList sourceUris, QObject *parent) : FileOperation(parent)
{
    mSourceUris = sourceUris;
    mReporter = new FileNodeReporter;
    mInfo = std::make_shared<FileOperationInfo>(sourceUris, nullptr, FileOperationInfo::Delete);
    connect(mReporter, &FileNodeReporter::nodeFound, this, &FileOperation::operationPreparedOne);
}

FileDeleteOperation::~FileDeleteOperation()
{
    delete mReporter;
}

void FileDeleteOperation::run()
{
    if (isCancelled()) {
        return;
    }

    Q_EMIT operationStarted();

    Q_EMIT operationRequestShowWizard();

    goffset *totalSize = new goffset(0);

    QList<FileNode*> nodes;
    for (auto uri : mSourceUris) {
        FileNode *node = new FileNode(uri, nullptr, mReporter);
        node->findChildrenRecursively();
        node->computeTotalSize(totalSize);
        nodes<<node;
    }
    operationPrepared();

    mTotalSize = *totalSize;
    delete totalSize;

    operationProgressed();

    for (auto node : nodes) {
        deleteRecursively(node);
    }

    Q_EMIT operationFinished();
}

void FileDeleteOperation::slotCancel()
{
    if (mReporter) {
        mReporter->slotCancel();
    }
    FileOperation::slotCancel();
}

void FileDeleteOperation::deleteRecursively(FileNode *node)
{
    if (isCancelled()) {
        return;
    }

    GFile *file = g_file_new_for_uri(node->uri().toUtf8().constData());
    if (node->isFolder()) {
        for (auto child : *(node->children())) {
            deleteRecursively(child);
        }
        GError *err = nullptr;
        g_file_delete(file,
                      getCancellable().get()->get(),
                      &err);
        if (err) {
            if (!mPrehandleHash.isEmpty()) {
                g_error_free(err);
                return;
            }
            //if delete a file get into error, it might be a critical error.
            auto response = errored(node->uri(), nullptr, GerrorWrapper::wrapFrom(err), true);
            auto responseType = response.value<ResponseType>();
            if (responseType == Cancel) {
                slotCancel();
            }
            if (responseType == IgnoreAll) {
                mPrehandleHash.insert(err->code, IgnoreAll);
            }
        }
    } else {
        GError *err = nullptr;
        g_file_delete(file, getCancellable().get()->get(), &err);
        if (err) {
            if (!mPrehandleHash.isEmpty()) {
                g_error_free(err);
                return;
            }

            auto response = errored(node->uri(), nullptr, GerrorWrapper::wrapFrom(err), true);
            auto responseType = response.value<ResponseType>();
            if (responseType == Cancel) {
                slotCancel();
            }
            if (responseType == IgnoreAll) {
                mPrehandleHash.insert(err->code, IgnoreAll);
            }
        }
    }
    g_object_unref(file);
    operationAfterProgressedOne(node->uri());
}

std::shared_ptr<FileOperationInfo> FileDeleteOperation::getOperationInfo()
{
    return mInfo;
}
