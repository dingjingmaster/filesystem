#ifndef FILEITEM_H
#define FILEITEM_H

#include <memory>
#include <QObject>
#include <file-watcher.h>

#include <file/file-enumerator.h>
#include <file/file-info.h>

class FileItemModel;

class FileItem : public QObject
{
    friend class FileItemModel;
    friend class FileItemProxyFilterSortModel;
    Q_OBJECT
public:
    explicit FileItem (std::shared_ptr<FileInfo> info, FileItem *parentItem = nullptr, FileItemModel *model = nullptr, QObject *parent = nullptr);
    ~FileItem ();

    bool hasChildren ();
    const QString uri ();
    void findChildrenAsync ();
    QModelIndex lastColumnIndex ();
    QModelIndex firstColumnIndex ();
    QVector<FileItem*> *findChildrenSync ();
    const std::shared_ptr<FileInfo> info ();
    bool operator == (const FileItem &item);

Q_SIGNALS:
    void cancelFindChildren ();
    void childAdded (const QString &uri);
    void deleted (const QString &thisUri);
    void childRemoved (const QString &uri);
    void renamed (const QString &oldUri, const QString &newUri);

public Q_SLOTS:
    void clearChildren ();
    void onUpdateDirectoryRequest ();
    void onChildAdded (const QString &uri);
    void onDeleted (const QString &thisUri);
    void onChildRemoved (const QString &uri);
    void onRenamed (const QString &oldUri, const QString &newUri);

protected:
    void updateInfoSync ();
    void updateInfoAsync ();
    FileItem *getChildFromUri (QString uri);

private:
    int                                 mAsyncCount = 0;
    bool                                mExpanded = false;

    std::shared_ptr<FileInfo>           mInfo;
    FileItemModel*                      mModel = nullptr;
    FileItem*                           mParent = nullptr;
    std::shared_ptr<FileWatcher>        mWatcher = nullptr;
    QVector<FileItem*>*                 mChildren = nullptr;
    FileEnumerator                      *mBackendEnumerator;
};

#endif // FILEITEM_H
