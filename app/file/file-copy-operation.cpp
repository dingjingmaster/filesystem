#include "file-copy-operation.h"
#include "file-enumerator.h"
#include "file-node-reporter.h"
#include "file-node.h"

#include <gobject/gerror-wrapper.h>

#include <QProcess>
#include <file-utils.h>

static void handleDuplicate (FileNode *node);

FileCopyOperation::FileCopyOperation(QStringList sourceUris, QString destDirUri, QObject *parent) : FileOperation (parent)
{
    if (sourceUris.first().contains(destDirUri)) {
        mIsDuplicatedCopy = true;
    }
    mSourceUris = sourceUris;
    mDestDirUri = destDirUri;
    mReporter = new FileNodeReporter;
    connect(mReporter, &FileNodeReporter::nodeFound, this, &FileOperation::operationPreparedOne);

    mInfo = std::make_shared<FileOperationInfo>(sourceUris, destDirUri, FileOperationInfo::Copy);
}

FileCopyOperation::~FileCopyOperation()
{
    delete mReporter;
}

void FileCopyOperation::run()
{
    if (isCancelled()) {
        return;
    }

    Q_EMIT operationStarted();

    Q_EMIT operationRequestShowWizard();

    goffset *total_size = new goffset(0);

    QList<FileNode*> nodes;
    for (auto uri : mSourceUris) {
        FileNode *node = new FileNode(uri, nullptr, mReporter);
        node->findChildrenRecursively();
        node->computeTotalSize(total_size);
        nodes<<node;
    }
    Q_EMIT operationPrepared();

    mTotalSize = *total_size;
    delete total_size;

    for (auto node : nodes) {
        copyRecursively(node);
    }
    Q_EMIT operationProgressed();

    if (isCancelled() && !hasError()) {
        Q_EMIT operationStartRollbacked();
        for (auto file : nodes) {
            if (isCancelled()) {
                rollbackNodeRecursively(file);
            }
        }
    }

    setHasError(false);

    for (auto node : nodes) {
        if (!isCancelled())
            mInfo->mNodeMap.insert(node->uri(), node->destUri());
        delete node;
    }

    nodes.clear();

    // judge if the operation should sync.
    bool needSync = false;
    GFile *src_first_file = g_file_new_for_uri(mSourceUris.first().toUtf8().constData());
    GMount *src_first_mount = g_file_find_enclosing_mount(src_first_file, nullptr, nullptr);
    if (src_first_mount) {
        needSync = g_mount_can_unmount(src_first_mount);
        g_object_unref(src_first_mount);
    } else {
        // maybe a vfs file.
        needSync = true;
    }
    g_object_unref(src_first_file);

    GFile *dest_dir_file = g_file_new_for_uri(mDestDirUri.toUtf8().constData());
    GMount *dest_dir_mount = g_file_find_enclosing_mount(dest_dir_file, nullptr, nullptr);
    if (src_first_mount) {
        needSync = g_mount_can_unmount(dest_dir_mount);
        g_object_unref(dest_dir_mount);
    } else {
        needSync = true;
    }
    g_object_unref(dest_dir_file);

    if (needSync) {
        operationStartSnyc();
        QProcess p;
        p.start("sync");
        p.waitForFinished(-1);
    }

    Q_EMIT operationFinished();
}

std::shared_ptr<FileOperationInfo> FileCopyOperation::getOperationInfo()
{
    return mInfo;
}

void FileCopyOperation::slotCancel()
{
    if (mReporter) {
        mReporter->slotCancel();
    }
    FileOperation::slotCancel();
}

FileOperation::ResponseType FileCopyOperation::prehandle(GError *err)
{
    setHasError(true);
    if (mIsDuplicatedCopy) {
        return BackupAll;
    }

    if (mPrehandleHash.contains(err->code)) {
        return mPrehandleHash.value(err->code);
    }

    return Other;
}

void FileCopyOperation::copyRecursively(FileNode *node)
{
    if (isCancelled()) {
        return;
    }

    node->setState(FileNode::Handling);

fallback_retry:
    QString destFileUri = node->resoveDestFileUri(mDestDirUri);
    node->setDestUri(destFileUri);
    GFileWrapperPtr destFile = wrapGFile(g_file_new_for_uri(destFileUri.toUtf8().constData()));

    mCurrentSrcUri = node->uri();
    mCurrentDestDirUri = destFileUri;

    if (node->isFolder()) {
        GError *err = nullptr;

        g_file_make_directory(destFile.get()->get(), getCancellable().get()->get(), &err);
        if (err) {
            if (err->code == G_IO_ERROR_CANCELLED) {
                return;
            }
            auto errWrapperPtr = GerrorWrapper::wrapFrom(err);
            ResponseType handleType = prehandle(err);
            if (handleType == Other) {
                auto typeData = errored(mCurrentSrcUri, mCurrentDestDirUri, errWrapperPtr);
                handleType = typeData.value<ResponseType>();
            }
            switch (handleType) {
            case IgnoreOne: {
                node->setState(FileNode::Unhandled);
                node->setErrorResponse(IgnoreOne);
                break;
            }
            case IgnoreAll: {
                node->setState(FileNode::Unhandled);
                node->setErrorResponse(IgnoreOne);
                mPrehandleHash.insert(err->code, IgnoreOne);
                break;
            }
            case OverWriteOne: {
                node->setState(FileNode::Handled);
                node->setErrorResponse(OverWriteOne);
                //make dir has no overwrite
                break;
            }
            case OverWriteAll: {
                node->setState(FileNode::Handled);
                node->setErrorResponse(OverWriteOne);
                mPrehandleHash.insert(err->code, OverWriteOne);
                break;
            }
            case BackupOne: {
                node->setState(FileNode::Handled);
                node->setErrorResponse(BackupOne);
                while (FileUtils::isFileExsit(node->resoveDestFileUri(mDestDirUri))) {
                    handleDuplicate(node);
                }
                goto fallback_retry;
            }
            case BackupAll: {
                node->setState(FileNode::Handled);
                node->setErrorResponse(BackupOne);
                while (FileUtils::isFileExsit(node->resoveDestFileUri(mDestDirUri))) {
                    handleDuplicate(node);
                }
                mPrehandleHash.insert(err->code, BackupOne);
                goto fallback_retry;
            }
            case Retry: {
                goto fallback_retry;
            }
            case Cancel: {
                node->setState(FileNode::Handled);
                slotCancel();
                break;
            }
            default:
                break;
            }
        } else {
            node->setState(FileNode::Handled);
        }
        mCurrentOffset += node->size();
        Q_EMIT operationProgressedOne(node->uri(), node->destUri(), node->size());
        for (auto child : *(node->children())) {
            copyRecursively(child);
        }
    } else {
        GError *err = nullptr;
        GFileWrapperPtr sourceFile = wrapGFile(g_file_new_for_uri(node->uri().toUtf8().constData()));
        g_file_copy(sourceFile.get()->get(), destFile.get()->get(), mDefaultCopyFlag, getCancellable().get()->get(), GFileProgressCallback(progressCallback), this, &err);
        if (err) {
            if (err->code == G_IO_ERROR_CANCELLED) {
                return;
            }
            auto errWrapperPtr = GerrorWrapper::wrapFrom(err);
            ResponseType handle_type = prehandle(err);
            if (handle_type == Other) {
                auto typeData = errored(mCurrentSrcUri, mCurrentDestDirUri, errWrapperPtr);
                handle_type = typeData.value<ResponseType>();
            }
            switch (handle_type) {
            case IgnoreOne: {
                node->setState(FileNode::Unhandled);
                node->setErrorResponse(IgnoreOne);
                break;
            }
            case IgnoreAll: {
                node->setState(FileNode::Unhandled);
                node->setErrorResponse(IgnoreOne);
                mPrehandleHash.insert(err->code, IgnoreOne);
                break;
            }
            case OverWriteOne: {
                g_file_copy(sourceFile.get()->get(), destFile.get()->get(), GFileCopyFlags(mDefaultCopyFlag | G_FILE_COPY_OVERWRITE), getCancellable().get()->get(), GFileProgressCallback(progressCallback), this, nullptr);
                node->setState(FileNode::Handled);
                node->setErrorResponse(OverWriteOne);
                break;
            }
            case OverWriteAll: {
                g_file_copy(sourceFile.get()->get(), destFile.get()->get(), GFileCopyFlags(mDefaultCopyFlag | G_FILE_COPY_OVERWRITE), getCancellable().get()->get(), GFileProgressCallback(progressCallback), this, nullptr);
                node->setState(FileNode::Handled);
                node->setErrorResponse(OverWriteOne);
                mPrehandleHash.insert(err->code, OverWriteOne);
                break;
            }
            case BackupOne: {
                node->setState(FileNode::Handled);
                node->setErrorResponse(BackupOne);
                while (FileUtils::isFileExsit(node->resoveDestFileUri(mDestDirUri))) {
                    handleDuplicate(node);
                }
                goto fallback_retry;
            }
            case BackupAll: {
                node->setState(FileNode::Handled);
                node->setErrorResponse(BackupOne);
                while (FileUtils::isFileExsit(node->resoveDestFileUri(mDestDirUri))) {
                    handleDuplicate(node);
                }
                mPrehandleHash.insert(err->code, BackupOne);
                goto fallback_retry;
            }
            case Retry: {
                goto fallback_retry;
            }
            case Cancel: {
                node->setState(FileNode::Handled);
                slotCancel();
                break;
            }
            default:
                break;
            }
        } else {
            node->setState(FileNode::Handled);
        }
        mCurrentOffset += node->size();
        Q_EMIT operationProgressedOne(node->uri(), node->destUri(), node->size());
    }
    destFile.reset();
}

void FileCopyOperation::rollbackNodeRecursively(FileNode *node)
{
    switch (node->state()) {
    case FileNode::Handling:
    case FileNode::Handled: {
        if (node->responseType() != Other) {
            break;
        }

        if (node->isFolder()) {
            auto children = node->children();
            for (auto child : *children) {
                rollbackNodeRecursively(child);
            }
            GFile *dest_file = g_file_new_for_uri(node->destUri().toUtf8().constData());
            bool is_folder_deleted = g_file_delete(dest_file, nullptr, nullptr);
            if (!is_folder_deleted) {
                FileEnumerator e;
                e.setEnumerateDirectory(node->destUri());
                e.enumerateSync();
                for (auto folder_child : *node->children()) {
                    if (!folder_child->destUri().isEmpty()) {
                        GFile *tmp_file = g_file_new_for_uri(folder_child->destUri().toUtf8().constData());
                        g_file_delete(tmp_file, nullptr, nullptr);
                        g_object_unref(tmp_file);
                    }
                    g_file_delete(dest_file, nullptr, nullptr);
                }
            }
            g_object_unref(dest_file);
        } else {
            GFile *dest_file = g_file_new_for_uri(node->destUri().toUtf8().constData());
            g_file_delete(dest_file, nullptr, nullptr);
            g_object_unref(dest_file);
        }
        operationRollbackedOne(node->destUri(), node->uri());
        break;
    }
    default: {
        if (node->isFolder()) {
            auto children = node->children();
            for (auto child : *children) {
                rollbackNodeRecursively(child);
            }
        }
        break;
    }
    }
}

void FileCopyOperation::progressCallback(goffset currentNumBytes, goffset totalNumBytes, FileCopyOperation *pThis)
{
    if (totalNumBytes < currentNumBytes) {
        return;
    }

    auto currnet = pThis->mCurrentOffset + currentNumBytes;
    auto total = pThis->mTotalSize;
    Q_EMIT pThis->FileProgressCallback(pThis->mCurrentSrcUri, pThis->mCurrentDestDirUri, currnet, total);
}

static void handleDuplicate (FileNode *node)
{
    QString name = node->destBaseName();
    QRegExp regExp("\\(\\d+\\)");
    if (name.contains(regExp)) {
        int pos = 0;
        int num = 0;
        QString tmp;
        while ((pos = regExp.indexIn(name, pos)) != -1) {
            tmp = regExp.cap(0).toUtf8();
            pos += regExp.matchedLength();
        }
        tmp.remove(0,1);
        tmp.chop(1);
        num = tmp.toInt();

        num++;
        name = name.replace(regExp, QString("(%1)").arg(num));
        node->setDestFileName(name);
    } else {
        if (name.contains(".")) {
            auto list = name.split(".");
            if (list.count() <= 1) {
                node->setDestFileName(name+"(1)");
            } else {
                int pos = list.count() - 1;
                if (list.last() == "gz" | list.last() == "xz" | list.last() == "Z" | list.last() == "sit" | list.last() == "bz" | list.last() == "bz2") {
                    pos--;
                }
                if (pos < 0) {
                    pos = 0;
                }
                auto tmp = list;
                QStringList suffixList;
                for (int i = 0; i < list.count() - pos; i++) {
                    suffixList.prepend(tmp.takeLast());
                }
                auto suffix = suffixList.join(".");

                auto basename = tmp.join(".");
                name = basename + "(1)" + "." + suffix;
                if (name.endsWith(".")) {
                    name.chop(1);
                }
                node->setDestFileName(name);
            }
        } else {
            name = name + "(1)";
            node->setDestFileName(name);
        }
    }
}
