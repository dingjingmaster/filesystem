#ifndef FILERENAMEOPERATION_H
#define FILERENAMEOPERATION_H

#include "file-operation.h"



class FileRenameOperation : public FileOperation
{
    Q_OBJECT
public:
    explicit FileRenameOperation (QString uri, QString newName);

    void run () override;
    std::shared_ptr<FileOperationInfo> getOperationInfo () override;

private:
    QString mUri = nullptr;
    QString mNewName = nullptr;
    std::shared_ptr<FileOperationInfo> mInfo = nullptr;
    GFileCopyFlags mdefaultCopyFlag = GFileCopyFlags(G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA);
};

#endif // FILERENAMEOPERATION_H
