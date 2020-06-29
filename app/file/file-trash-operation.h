#ifndef FILETRASHOPERATION_H
#define FILETRASHOPERATION_H

#include "file-operation.h"



class FileTrashOperation : public FileOperation
{
    Q_OBJECT
public:
    explicit FileTrashOperation (QStringList srcUris, QObject *parent = nullptr);

    void run () override;
    std::shared_ptr<FileOperationInfo> getOperationInfo () override;

private:
    QStringList mSrcUris;
    std::shared_ptr<FileOperationInfo> mInfo = nullptr;
};

#endif // FILETRASHOPERATION_H
