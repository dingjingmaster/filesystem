#ifndef FILEOPERATIONUTILS_H
#define FILEOPERATIONUTILS_H

#include <memory>
#include <QStringList>

#include <file/file-create-templ-operation.h>

class FileInfo;

class FileOperationUtils
{
public:
    static void remove (const QStringList &uris);
    static void restore (const QString &uriInTrash);
    static void restore (const QStringList &urisInTrash);
    static void trash (const QStringList &uris, bool addHistory);
    static std::shared_ptr<FileInfo> queryFileInfo(const QString &uri);
    static void executeRemoveActionWithDialog (const QStringList &uris);
    static void rename (const QString &uri, const QString &newName, bool addHistory);
    static void link (const QString &srcUri, const QString &destUri, bool addHistory);
    static bool leftNameLesserThanRightName (const QString &left, const QString &right);
    static void copy (const QStringList &srcUris, const QString &destUri, bool addHistory);
    static bool leftNameIsDuplicatedFileOfRightName (const QString &left, const QString &right);
    static void move (const QStringList &srcUris, const QString &destUri, bool addHistory, bool copyMove = false);
    static void create (const QString &destDirUri, const QString &name = nullptr, FileCreateTemplOperation::Type type = FileCreateTemplOperation::EmptyFile);

private:
    FileOperationUtils();
};

#endif // FILEOPERATIONUTILS_H
