#include "file-operation-manager.h"

#include <QVariant>
#include <QMessageBox>
#include <QApplication>
#include <QtConcurrent>
#include <QStandardPaths>

#include <QUrl>
#include <file-utils.h>
#include <file-watcher.h>

static FileOperationManager *globalInstance = nullptr;

FileOperationManager *FileOperationManager::getInstance()
{
    if (globalInstance == nullptr) {
        globalInstance = new FileOperationManager;
    }
    return globalInstance;
}

void FileOperationManager::close()
{
    disconnect();
    deleteLater();
    globalInstance = nullptr;
    Q_EMIT closed();
}

bool FileOperationManager::isAllowParallel()
{
    return mAllowParallel;
}

/**
 *
 * @note FIXME://
 */
void FileOperationManager::setAllowParallel(bool allow)
{
    mAllowParallel = allow;
    if (allow) {
        mThreadPool->setMaxThreadCount(9999);
    } else {
        mThreadPool->setMaxThreadCount(1);
    }

//    GlobalSettings::getInstance()->setValue(ALLOW_FILE_OP_PARALLEL, allow);
}

void FileOperationManager::slotUndo()
{
    if(!slotCanUndo())
        return;

    auto undoInfo = mUndoStack.pop();
    mRedoStack.push(undoInfo);

    auto oppositeInfo = undoInfo->getOppositeInfo(*undoInfo.get());
    slotStartUndoOrRedo(oppositeInfo);
}

void FileOperationManager::slotRedo()
{
    if (!slotCanRedo())
        return;

    auto redoInfo = mRedoStack.pop();
    mUndoStack.push(redoInfo);

    slotStartUndoOrRedo(redoInfo);
}

bool FileOperationManager::slotCanRedo()
{
    return !mRedoStack.isEmpty();
}

bool FileOperationManager::slotCanUndo()
{
    return !mUndoStack.isEmpty();
}

void FileOperationManager::slotClearHistory()
{
    mUndoStack.clear();
    mRedoStack.clear();
}

void FileOperationManager::registerFileWatcher(FileWatcher *watcher)
{
    mWatchers << watcher;
}

void FileOperationManager::unregisterFileWatcher(FileWatcher *watcher)
{
    mWatchers.removeOne(watcher);
}

void FileOperationManager::slotOnFilesDeleted(const QStringList &uris)
{
    slotClearHistory();
}

std::shared_ptr<FileOperationInfo> FileOperationManager::slotGetRedoInfo()
{
    return mRedoStack.top();
}

std::shared_ptr<FileOperationInfo> FileOperationManager::slotGetUndoInfo()
{
    return mUndoStack.top();
}

void FileOperationManager::manuallyNotifyDirectoryChanged(FileOperationInfo *info)
{
    if (!info)
        return;

    // skip create template opeartion, it will be handled by operation itself.
    if (info->mSrcDirUri == QStandardPaths::writableLocation(QStandardPaths::TempLocation))
        return;

    for (auto watcher : mWatchers) {
        if (!watcher->supportMonitor()) {
            auto srcDir = info->mSrcDirUri;
            auto destDir = info->mDestDirUri;
            if (info->operationType() == FileOperationInfo::Link || info->operationType() == FileOperationInfo::Rename) {
                srcDir = FileUtils::getParentUri(info->mSrcUris.first());
            }
            // check watcher directory
            if (watcher->currentUri() == srcDir || watcher->currentUri() == destDir) {
                // tell the view/model the directory should be updated
                watcher->requestUpdateDirectory();
            }
        }
    }
}

void FileOperationManager::slotStartUndoOrRedo(std::shared_ptr<FileOperationInfo> info)
{
    FileOperation *op = nullptr;
    switch (info->mType) {
    case FileOperationInfo::Copy: {
        op = new FileCopyOperation(info->mSrcUris, info->mDestDirUri);
        break;
    }
    case FileOperationInfo::Delete: {
        if (info->mNodeMap.isEmpty())
            op = new FileDeleteOperation(info->mSrcUris);
        else
            op = new FileDeleteOperation(info->mNodeMap.keys());
        break;
    }
    case FileOperationInfo::Link: {
        op = new FileDeleteOperation(info->mSrcUris);
        break;
    }
    case FileOperationInfo::Move: {
        op = new FileMoveOperation(info->mSrcUris, info->mDestDirUri);
        break;
    }
    case FileOperationInfo::Rename: {
        if (info->mNodeMap.isEmpty()) {
            op = new FileRenameOperation(info->mSrcUris.isEmpty()? nullptr: info->mSrcUris.at(0), info->mDestDirUri);
        } else {
            auto destUri = info->mNodeMap.first();
            QUrl url = destUri;
            op = new FileRenameOperation(info->mNodeMap.firstKey(), url.fileName());
        }
        break;
    }
    case FileOperationInfo::Trash: {
        op = new FileTrashOperation(info->mSrcUris);
        break;
    }
    case FileOperationInfo::Untrash: {
        op = new FileUntrashOperation(info->mSrcUris);
        break;
    }
    default:
        break;
    }
    //do not record the undo/redo operation to history again.
    //this had been handled at undo() and redo() yet.
    //FIXME: if an undo/redo work went error (usually won't),
    //should i remove the operation info from stack?
    if (op) {
        slotStartOperation(op, false);
    }
}

void FileOperationManager::slotStartOperation(FileOperation *operation, bool addToHistory)
{
    QApplication::setQuitOnLastWindowClosed(false);

    connect(operation, &FileOperation::operationFinished, this, [=]() {
        operation->notifyFileWatcherOperationFinished();
        auto settings = GlobalSettings::getInstance();
        bool runbackend = settings->getInstance()->getValue(RESIDENT_IN_BACKEND).toBool();
        QApplication::setQuitOnLastWindowClosed(!runbackend);
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
        QTimer::singleShot(1000, this, [=]() {
#else
        QTimer::singleShot(1000, [=]() {
#endif
            int lastOpCount = mThreadPool->children().count();
            if (lastOpCount == 0) {
                if (qApp->allWidgets().isEmpty()) {
                    if (!runbackend) {
                        qApp->quit();
                    }
                }
            }
        });
    });

    auto operationInfo = operation->getOperationInfo();

    bool allowParallel = mAllowParallel;

    auto opType = operationInfo->operationType();
    switch (opType) {
    case FileOperationInfo::Trash:
    case FileOperationInfo::Delete: {
        allowParallel = true;
        auto operationSrcs = operationInfo->sources();
        auto currentOps = mThreadPool->children();
        QList<FileOperation *> ops;
        for (auto child : currentOps) {
            auto op = qobject_cast<FileOperation *>(child);
            auto opInfo = op->getOperationInfo();
            {
                for (auto src : operationSrcs) {
                    if (opInfo->sources().contains(src)) {
                        //do not allow operation.
                        QMessageBox::critical(nullptr,
                                              tr("Can't delete."),
                                              tr("You can't delete a file when"
                                                 "the file is doing another operation"));
                        return;
                    }
                }
            }
        }
        break;
    }
    default:
        break;
    }

    FileOperationProgressWizard *wizard = new FileOperationProgressWizard;
    wizard->setAttribute(Qt::WA_DeleteOnClose);
    wizard->connect(operation, &FileOperation::operationRequestShowWizard, wizard, &FileOperationProgressWizard::delayShow);
    wizard->connect(operation, &FileOperation::operationRequestShowWizard, wizard, &FileOperationProgressWizard::switchToPreparedPage);
    wizard->connect(operation, &FileOperation::operationPreparedOne, wizard, &FileOperationProgressWizard::onElementFoundOne);
    wizard->connect(operation, &FileOperation::operationPrepared, wizard, &FileOperationProgressWizard::onElementFoundAll);
    wizard->connect(operation, &FileOperation::operationProgressedOne, wizard, &FileOperationProgressWizard::onFileOperationProgressedOne);
    wizard->connect(operation, &FileOperation::FileProgressCallback, wizard, &FileOperationProgressWizard::updateProgress);
    wizard->connect(operation, &FileOperation::operationProgressed, wizard, &FileOperationProgressWizard::onFileOperationProgressedAll);
    wizard->connect(operation, &FileOperation::operationAfterProgressedOne, wizard, &FileOperationProgressWizard::onElementClearOne);
    wizard->connect(operation, &FileOperation::operationAfterProgressed, wizard, &FileOperationProgressWizard::switchToRollbackPage);
    wizard->connect(operation, &FileOperation::operationStartRollbacked, wizard, &FileOperationProgressWizard::switchToRollbackPage);
    wizard->connect(operation, &FileOperation::operationRollbackedOne, wizard, &FileOperationProgressWizard::onFileRollbacked);
    wizard->connect(operation, &FileOperation::operationStartSnyc, wizard, &FileOperationProgressWizard::onStartSync);
    wizard->connect(operation, &FileOperation::operationFinished, wizard, &FileOperationProgressWizard::deleteLater);

    connect(wizard, &FileOperationProgressWizard::cancelled,
            operation, &FileOperation::cancel);

    operation->connect(operation, &FileOperation::errored, [=]() {
        operation->setHasError(true);
    });

    operation->connect(operation, &FileOperation::errored,
                       this, &FileOperationManager::handleError,
                       Qt::BlockingQueuedConnection);

    operation->connect(operation, &FileOperation::operationFinished, [=]() {
        if (operation->hasError()) {
            this->slotClearHistory();
            return ;
        }

        if (addToHistory) {
            auto info = operation->getOperationInfo();
            if (!info)
                return;
            if (info->operationType() != FileOperationInfo::Delete) {
                mUndoStack.push(info);
                mRedoStack.clear();
            } else {
                this->slotClearHistory();
            }
        }
    });

    if (!allowParallel) {
        if (mThreadPool->activeThreadCount() > 0) {
            QMessageBox::warning(nullptr,
                                 tr("File Operation is Busy"),
                                 tr("There have been one or more file"
                                    "operation(s) executing before. Your"
                                    "operation will wait for executing"
                                    "until it/them done. If you really "
                                    "want to execute file operations "
                                    "parallelly anyway, you can change "
                                    "the default option \"Allow Parallel\" "
                                    "in option menu."));
        }
        operation->setParent(mThreadPool);
        mThreadPool->start(operation);
    } else {
        QtConcurrent::run([=] {
            operation->setParent(mThreadPool);
            operation->setAutoDelete(false);
            operation->run();
            operation->setParent(nullptr);
            operation->deleteLater();
        });
    }
}

QVariant FileOperationManager::handleError(const QString &srcUri, const QString &destUri, const GerrorWrapperPtr &err, bool critical)
{
    FileOperationErrorDialog dlg;
    return dlg.handleError(srcUri, destUri, err, critical);
}

FileOperationManager::FileOperationManager(QObject *parent) : QObject(parent)
{
    mAllowParallel = GlobalSettings::getInstance()->getValue(ALLOW_FILE_OP_PARALLEL).toBool();

    qRegisterMetaType<GerrorWrapperPtr>("GerrorWrapperPtr");
    qRegisterMetaType<GerrorWrapperPtr>("GerrorWrapperPtr&");
    mThreadPool = new QThreadPool(this);

    if (!mAllowParallel) {
        mThreadPool->setMaxThreadCount(1);
    }
}
