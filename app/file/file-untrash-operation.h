#ifndef FILEUNTRASHOPERATION_H
#define FILEUNTRASHOPERATION_H

#include "file-operation.h"

#include <QHash>



class FileUntrashOperation : public FileOperation
{
    Q_OBJECT
public:
    explicit FileUntrashOperation (QStringList uris, QObject *parent = nullptr);

    void run () override;
    std::shared_ptr<FileOperationInfo> getOperationInfo () override;

protected:
    void cacheOriginalUri ();
    const QString handleDuplicate (const QString &uri);

private:
    QStringList mUris;
    QHash<QString, QString> mRestoreHash;
    ResponseType mPreHandler = Invalid;
    std::shared_ptr<FileOperationInfo> mInfo = nullptr;
    GFileCopyFlags mDefaultCopyFlag = GFileCopyFlags(G_FILE_COPY_NOFOLLOW_SYMLINKS | G_FILE_COPY_ALL_METADATA);
};

#endif // FILEUNTRASHOPERATION_H
