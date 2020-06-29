#ifndef FILECOPYOPERATION_H
#define FILECOPYOPERATION_H

#include "file-operation.h"

#include <QHash>

class FileNodeReporter;
class FileNode;

class FileCopyOperation : public FileOperation
{
    Q_OBJECT
public:
    explicit FileCopyOperation (QStringList sourceUris, QString destDirUri, QObject *parent = nullptr);
    ~FileCopyOperation () override;

    void run () override;
    std::shared_ptr<FileOperationInfo> getOperationInfo () override;

public Q_SLOTS:
    void slotCancel () override;

protected:
    ResponseType prehandle (GError *err);
    void copyRecursively (FileNode *node);
    void rollbackNodeRecursively (FileNode *node);
    static void progressCallback (goffset currentNumBytes, goffset totalNumBytes, FileCopyOperation *pThis);

private:
    int mTotalCount = 0;
    int mCurrentCount = 0;
    goffset mTotalSize = 0;
    goffset mCurrentOffset = 0;

    QStringList mSourceUris;
    bool mIsDuplicatedCopy = false;
    QString mDest_dirUri = nullptr;
    QString mCurrentSrcUri = nullptr;
    QString mCurrentDestDirUri = nullptr;
    FileNodeReporter *mReporter = nullptr;
    QHash<int, ResponseType> mPrehandleHash;
    std::shared_ptr<FileOperationInfo> mInfo = nullptr;

    GFileCopyFlags mDefaultCopyFlag = GFileCopyFlags(G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA);
};

#endif // FILECOPYOPERATION_H
