#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <glib.h>
#include <glib/gerror.h>
#include <gobject/gerror-wrapper.h>
#include <gobject/gobject-template.h>

#include <QString>
#include <QStringList>


class FileUtils
{
public:
    static bool isMountRoot(const QString &uri);
    static bool isFileExsit(const QString &uri);
    static bool isFileDirectory(const QString &uri);
    static QString getTargetUri(const QString &uri);
    static bool getFileIsFolder(const QString &uri);
    static bool isFileUnmountable(const QString &uri);
    static QString getUriBaseName(const QString &uri);
    static QString getFileIconName(const QString &uri);
    static const QString getParentUri(const QString &uri);
    static QString getFileDisplayName(const QString &uri);
    static QString getFileUri(const GFileWrapperPtr &file);
    static bool getFileIsFolder(const GFileWrapperPtr &file);
    static bool stringStartWithChinese(const QString &string);
    static QString getFileBaseName(const GFileWrapperPtr &file);
    static bool getFileHasChildren(const GFileWrapperPtr &file);
    static GerrorWrapperPtr getEnumerateError(const QString &uri);
    static const QStringList toDisplayUris(const QStringList &args);
    static QStringList getChildrenUris(const QString &directoryUri);
    static QString getNonSuffixedBaseNameFromUri(const QString &uri);
    static GFileWrapperPtr getFileParent(const GFileWrapperPtr &file);
    static QString getQStringFromCString(char *cstring, bool free = true);
    static bool stringLesserThan(const QString &left, const QString &right);
    static QString getRelativePath(const GFileWrapperPtr &dir, const GFileWrapperPtr &file);
    static GFileWrapperPtr resolveRelativePath(const GFileWrapperPtr &dir, const QString &relativePath);
    static bool queryVolumeInfo(const QString &volumeUri, QString &volumeName, QString &unixDeviceName, const QString &volumeDisplayName = nullptr);

private:
    FileUtils();
};

#endif // FILEUTILS_H
