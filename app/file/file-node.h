#ifndef FILENODE_H
#define FILENODE_H

#include <QList>
#include <memory>
#include <QString>
#include <gio/gio.h>

#include "file-operation.h"

class FileNodeReporter;

class FileNode
{
    friend class FileNodeReporter;
public:
    enum State {
        Unhandled,
        Handling,
        Handled,
        Cleared,
        Invalid
    };

    FileNode (QString uri, FileNode* parent, FileNodeReporter *reporter = nullptr);
    ~FileNode ();

    qint64 size ();
    QString uri ();
    State state ();
    bool isFolder ();
    QString destUri ();
    QString baseName ();
    FileNode *parent ();
    QString getRelativePath ();
    void setState (State state);
    void setDestUri(QString uri);
    QList<FileNode*> *children ();
    const QString destBaseName ();
    void findChildrenRecursively ();
    void computeTotalSize (goffset *offset);
    void setDestFileName (const QString &name);
    FileOperation::ResponseType responseType ();
    void setErrorResponse (FileOperation::ResponseType type);
    const QString resoveDestFileUri(const QString &destRootDir);

private:
    goffset mSize = 0;
    bool mIsFolder = false;
    QString mUri = nullptr;
    State mState = Unhandled;
    QString mDestUri = nullptr;
    FileNode *mParent = nullptr;
    QString mBasename = nullptr;
    QString mDestBasename = nullptr;
    FileNodeReporter *mReporter = nullptr;
    QList<FileNode*> *mChildren = nullptr;
    FileOperation::ResponseType mErrResponse = FileOperation::Other;
};

#endif // FILENODE_H
