#ifndef FILELINKOPERATION_H
#define FILELINKOPERATION_H

#include "file-operation.h"



class FileLinkOperation : public FileOperation
{
    Q_OBJECT
public:
    FileLinkOperation (QString srcUri, QString destDirUri, QObject *parent = nullptr);
    ~FileLinkOperation () override;

    void run () override;
    std::shared_ptr<FileOperationInfo> getOperationInfo () override;

private:
    QString mSrcUri = nullptr;
    QString mDestUri = nullptr;
    std::shared_ptr<FileOperationInfo> mInfo = nullptr;
};

#endif // FILELINKOPERATION_H
