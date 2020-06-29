#ifndef FILECREATETEMPLOPERATION_H
#define FILECREATETEMPLOPERATION_H

#include "file-operation.h"

#include <QObject>

class FileCreateTemplOperation : public FileOperation
{
    Q_OBJECT
public:
    enum Type
    {
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
    Type mType;
    QString mSrcUri;
    QString mTargetUri;
    QString mDestDirUri;
    std::shared_ptr<FileOperationInfo> mInfo;
};

#endif // FILECREATETEMPLOPERATION_H
