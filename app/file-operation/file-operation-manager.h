#ifndef FILEOPERATIONMANAGER_H
#define FILEOPERATIONMANAGER_H

#include "file-operation.h"
#include "file-operation-info.h"
#include "gobject/gerror-wrapper.h"
#include "gobject/gobject-template.h"

#include <memory>
#include <QStack>
#include <QObject>
#include <QVariant>
#include <QThreadPool>

class FileWatcher;

class FileOperationManager : public QObject
{
    Q_OBJECT
public:
    static FileOperationManager* getInstance ();

    void close ();
    bool isAllowParallel ();
    void setAllowParallel (bool allow = true);

Q_SIGNALS:
    void closed ();

public Q_SLOTS:
    void slotUndo();
    void slotRedo();
    bool slotCanRedo();
    bool slotCanUndo();
    void slotClearHistory();
    void registerFileWatcher(FileWatcher *watcher);
    void unregisterFileWatcher(FileWatcher *watcher);
    void slotOnFilesDeleted(const QStringList &uris);
    std::shared_ptr<FileOperationInfo> slotGetRedoInfo();
    std::shared_ptr<FileOperationInfo> slotGetUndoInfo();
    void manuallyNotifyDirectoryChanged (FileOperationInfo *info);
    void slotStartUndoOrRedo (std::shared_ptr<FileOperationInfo> info);
    void slotStartOperation (FileOperation *operation, bool addToHistory = true);
    QVariant slotHandleError (const QString &srcUri, const QString &destUri, const GerrorWrapperPtr &err, bool critical);

private:
    explicit FileOperationManager (QObject *parent = nullptr);
    ~FileOperationManager ();

    QThreadPool *mThreadPool;
    bool mAllowParallel = false;
    QVector<FileWatcher *> mWatchers;
    bool mIsCurrentOperationErrored = false;
    QStack<std::shared_ptr<FileOperationInfo>> mUndoStack;
    QStack<std::shared_ptr<FileOperationInfo>> mRedoStack;
};

#endif // FILEOPERATIONMANAGER_H
