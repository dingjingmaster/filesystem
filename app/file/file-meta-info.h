#ifndef FILEMETAINFO_H
#define FILEMETAINFO_H
#include <QHash>
#include <memory>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <gio/gio.h>
#include <QStringList>

class FileMetaInfo
{
    friend class FileInfo;
    friend class FileInfoJob;
    friend class FileInfoManager;
public:
    FileMetaInfo(const QString &uri, GFileInfo *gInfo);

    int getMetaInfoInt(const QString &key);
    void removeMetaInfo(const QString &key);
    void setMetaInfoInt(const QString &key, int value);
    const QString getMetaInfoString(const QString &key);
    const QVariant getMetaInfoVariant(const QString &key);
    const QStringList getMetaInfoStringList(const QString &key);
    void setMetaInfoString(const QString &key, const QString &value);
    static std::shared_ptr<FileMetaInfo> fromUri (const QString &uri);
    void setMetaInfoVariant(const QString &key, const QVariant &value);
    void setMetaInfoStringList(const QString &key, const QStringList &value);
    static std::shared_ptr<FileMetaInfo> fromGFileInfo (const QString &uri, GFileInfo *gInfo);

private:
    QString mUri;
    QMutex mMutex;
    QHash<QString, QVariant> mMetaHash;
};


#endif // FILEMETAINFO_H
