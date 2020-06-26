#include "file-operation-manager.h"

#include <QVariant>

FileOperationManager::FileOperationManager()
{

}

void FileOperationManager::close()
{

}

bool FileOperationManager::isAllowParallel()
{

}

void FileOperationManager::setAllowParallel(bool allow)
{

}

void FileOperationManager::slotUndo()
{

}

void FileOperationManager::slotRedo()
{

}

bool FileOperationManager::slotCanRedo()
{

}

bool FileOperationManager::slotCanUndo()
{

}

void FileOperationManager::slotClearHistory()
{

}

void FileOperationManager::registerFileWatcher(FileWatcher *watcher)
{

}

void FileOperationManager::unregisterFileWatcher(FileWatcher *watcher)
{

}

void FileOperationManager::slotOnFilesDeleted(const QStringList &uris)
{

}

std::shared_ptr<FileOperationInfo> FileOperationManager::slotGetRedoInfo()
{

}

std::shared_ptr<FileOperationInfo> FileOperationManager::slotGetUndoInfo()
{

}

void FileOperationManager::manuallyNotifyDirectoryChanged(FileOperationInfo *info)
{

}

void FileOperationManager::slotStartUndoOrRedo(std::shared_ptr<FileOperationInfo> info)
{

}

void FileOperationManager::slotStartOperation(FileOperation *operation, bool addToHistory)
{

}

QVariant FileOperationManager::handleError(const QString &srcUri, const QString &destUri, const GerrorWrapperPtr &err, bool critical)
{

}
