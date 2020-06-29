#include "file-node.h"

FileNode::FileNode(QString uri, FileNode *parent, FileNodeReporter *reporter)
{

}

FileNode::~FileNode()
{

}

qint64 FileNode::size()
{
    return mSize;
}

QString FileNode::uri()
{
    return mUri;
}

FileNode::State FileNode::state()
{
    return mState;
}

bool FileNode::isFolder()
{
    return mIsFolder;
}

QString FileNode::destUri()
{
    return mDestUri;
}

QString FileNode::baseName()
{
    return mBasename;
}

FileNode *FileNode::parent()
{
    return mParent;
}

QString FileNode::getRelativePath()
{

}

void FileNode::setState(FileNode::State state)
{
    mState = state;
}

void FileNode::setDestUri(QString uri)
{
    mDestUri = uri;
}

QList<FileNode *> *FileNode::children()
{
    return mChildren;
}

const QString FileNode::destBaseName()
{
    return mDestBasename;
}

void FileNode::findChildrenRecursively()
{

}

void FileNode::computeTotalSize(goffset *offset)
{

}

void FileNode::setDestFileName(const QString &name)
{
    mDestBasename = name;
}

FileOperation::ResponseType FileNode::responseType()
{
    return mErrResponse;
}

void FileNode::setErrorResponse(FileOperation::ResponseType type)
{
    mErrResponse = type;
}

const QString FileNode::resoveDestFileUri(const QString &destRootDir)
{

}
