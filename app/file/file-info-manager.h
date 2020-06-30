#ifndef FILEINFOMANAGER_H
#define FILEINFOMANAGER_H

#include "file-info.h"

#include <QMutex>
#include <memory>


class FileInfoManager
{
    friend class FileInfo;
public:
    void lock ();
    void clear ();
    void unlock ();
    void showState ();
    void remove(QString uri);
    static FileInfoManager *getInstance ();
    void remove (std::shared_ptr<FileInfo> info);
    std::shared_ptr<FileInfo> findFileInfoByUri (QString uri);

protected:
    void removeFileInfobyUri (QString uri);
    std::shared_ptr<FileInfo> insertFileInfo (std::shared_ptr<FileInfo> info);

private:
    FileInfoManager ();
    ~FileInfoManager ();

    QMutex mMutex;
};

#endif // FILEINFOMANAGER_H
