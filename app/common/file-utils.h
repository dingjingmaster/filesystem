#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <glib.h>
#include <glib/gerror.h>

#include <QString>
#include <QStringList>


class FileUtils
{
public:
    static QString getQStringFromCString(char *cstring, bool free = true);
//    static QString getFileUri(const GFileWrapperPtr &file);
//    static QString getFileBaseName(const GFileWrapperPtr &file);
    static QString getUriBaseName(const QString &uri);
//    static GFileWrapperPtr getFileParent(const GFileWrapperPtr &file);
//    static QString getRelativePath(const GFileWrapperPtr &dir, const GFileWrapperPtr &file);
//    static GFileWrapperPtr resolveRelativePath(const GFileWrapperPtr &dir, const QString &relativePath);
//    static bool getFileHasChildren(const GFileWrapperPtr &file);
//    static bool getFileIsFolder(const GFileWrapperPtr &file);
    static bool getFileIsFolder(const QString &uri);
    static QStringList getChildrenUris(const QString &directoryUri);

    static QString getNonSuffixedBaseNameFromUri(const QString &uri);
    static QString getFileDisplayName(const QString &uri);
    static QString getFileIconName(const QString &uri);

//    static GErrorWrapperPtr getEnumerateError(const QString &uri);
    static QString getTargetUri(const QString &uri);

    static bool stringStartWithChinese(const QString &string);
    static bool stringLesserThan(const QString &left, const QString &right);

    static const QString getParentUri(const QString &uri);

    static bool isFileExsit(const QString &uri);

    static const QStringList toDisplayUris(const QStringList &args);

    static bool isMountRoot(const QString &uri);

    static bool queryVolumeInfo(const QString &volumeUri, QString &volumeName, QString &unixDeviceName, const QString &volumeDisplayName = nullptr);

    static bool isFileDirectory(const QString &uri);

    static bool isFileUnmountable(const QString &uri);

private:
    FileUtils();
};

#endif // FILEUTILS_H
