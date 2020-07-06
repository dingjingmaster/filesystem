#ifndef FILEOPEARTIONINFO_H
#define FILEOPEARTIONINFO_H

#include <QMap>
#include <QObject>
#include <memory>

class FileOperationInfo : public QObject
{
    Q_OBJECT
    friend class FileOperation;
    friend class FileOperationManager;

public:
    enum Type {
        Invalid,
        Move,               // move back if no error in original moving
        Copy,               // delete if no error in original copying
        Link,               // delete...
        Trash,              // untrash
        Rename,             // rename
        Delete,             // nothing to do
        Untrash,            // trash
        CreateTxt,          // delete
        CreateFolder,       // delete
        CreateTemplate,     // delete
        Other               // nothing to do
    };

    explicit FileOperationInfo (QStringList srcUris, QString destDirUri, Type type, QObject *parent = nullptr);

    QString target ();
    Type operationType ();
    QStringList sources ();
    std::shared_ptr<FileOperationInfo> getOppositeInfo (FileOperationInfo& info);

public:
    QMap<QString, QString>  mNodeMap;

private:
    bool                    mEnable = true;

    Type                    mType;
    Type                    mOppositeType;

    QString                 mSrcDirUri;
    QString                 mDestDirUri;
    QStringList             mSrcUris;
    QStringList             mDestUris;

    QString                 mOldName = nullptr;
    QString                 mNewName = nullptr;
};

#endif // FILEOPEARTIONINFO_H
