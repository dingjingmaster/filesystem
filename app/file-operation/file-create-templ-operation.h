#ifndef FILECREATETEMPLOPERATION_H
#define FILECREATETEMPLOPERATION_H

#include "file-operation.h"



class FileCreateTemplOperation : public FileOperation
{
public:
    enum Type {
        EmptyFile,
        EmptyFolder,
        Template
    };

    explicit FileCreateTemplOperation(const QString &destDirUri, Type type = EmptyFile, const QString &templateName = nullptr, QObject *parent = nullptr);

    void run () override;
    const QString target ();
    std::shared_ptr<FileOperationInfo> getOperationInfo () override;

protected:
    void handleDuplicate(const QString &uri);

private:
    std::shared_ptr<FileOperationInfo> mInfo;

    QString mSrcUri;
    QString mDestDirUri;
    QString mTargetUri;
    Type mType;
};

#endif // FILECREATETEMPLOPERATION_H
