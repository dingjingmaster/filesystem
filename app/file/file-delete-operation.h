#ifndef FILEDELETEOPERATION_H
#define FILEDELETEOPERATION_H

#include "file-operation.h"

#include <QHash>

class FileNode;
class FileNodeReporter;

class FileDeleteOperation : public FileOperation
{
    Q_OBJECT
public:
    explicit FileDeleteOperation (QStringList sourceUris, QObject *parent = nullptr);
    ~FileDeleteOperation () override;

    void run () override;
    void slotCancel () override;
    void deleteRecursively (FileNode *node);
    std::shared_ptr<FileOperationInfo> getOperationInfo () override;

private:
    int mTotalCount = 0;
    int mCurrentCount = 0;
    goffset mTotalSize = 0;
    goffset mCurrentOffset = 0;

    QStringList mSourceUris;
    QString mCurrentSrcUri = nullptr;
    FileNodeReporter *mReporter = nullptr;
    QHash<int, ResponseType> mPrehandleHash;
    std::shared_ptr<FileOperationInfo> mInfo = nullptr;
};

#endif // FILEDELETEOPERATION_H
