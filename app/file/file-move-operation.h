#ifndef FILEMOVEOPERATION_H
#define FILEMOVEOPERATION_H

#include "file-operation.h"

#include <QHash>

class FileNode;
class FileNodeReporter;
class FileOperationInfo;

class FileMoveOperation : public FileOperation
{
    Q_OBJECT
public:

    explicit FileMoveOperation (QStringList sourceUris, QString destDirUri, QObject *parent = nullptr);
    ~FileMoveOperation () override;

    void run () override;
    void setCopyMove (bool copyMove = true);
    void rollbackNodeRecursively (FileNode *node);
    void setForceUseFallback (bool useFallback = true);
    std::shared_ptr<FileOperationInfo> getOperationInfo() override;

public Q_SLOTS:
    void slotCancel() override;

protected:
    void move ();
    bool isValid ();
    void moveForceUseFallback ();
    ResponseType prehandle (GError *err);
    void copyRecursively (FileNode *node);
    void deleteRecursively (FileNode *node);
    static void progressCallback (goffset currentNumBytes, goffset totalNumBytes, FileMoveOperation *pThis);

private:
    int mTotalCount = 0;
    int mCurrentCount = 0;
    goffset mTotalSzie = 0;
    goffset mCurrentOffset = 0;

    bool mCopyMove = false;
    bool mForceUseFallback = false;

    QStringList mSourceUris;
    QString mDestDirUri = nullptr;
    QString mCurrentSrcUri = nullptr;
    QString mCurrentDestDirUri = nullptr;
    FileNodeReporter *mReporter = nullptr;
    QHash<int, ResponseType> mPrehandleHash;
    std::shared_ptr<FileOperationInfo> mInfo = nullptr;
    GFileCopyFlags mDefaultCopyFlag = GFileCopyFlags(G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA | G_FILE_COPY_NO_FALLBACK_FOR_MOVE);
};

#endif // FILEMOVEOPERATION_H
