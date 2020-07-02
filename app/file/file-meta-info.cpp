#include "file-info-manager.h"
#include "file-meta-info.h"

std::shared_ptr<FileMetaInfo> FileMetaInfo::fromGFileInfo(const QString &uri, GFileInfo *gInfo)
{
    return std::make_shared<FileMetaInfo>(uri, gInfo);
}

std::shared_ptr<FileMetaInfo> FileMetaInfo::fromUri(const QString &uri)
{
    auto mgr = FileInfoManager::getInstance();
    auto info = mgr->findFileInfoByUri(uri);
    if (info) {
        return mgr->findFileInfoByUri(uri)->mMetaInfo;
    }

    return nullptr;
}

FileMetaInfo::FileMetaInfo(const QString &uri, GFileInfo *gInfo)
{
    mUri = uri;
    if (gInfo) {
        char **metainfo_attributes = g_file_info_list_attributes(gInfo, "metadata");
        if (metainfo_attributes) {
            for (int i = 0; metainfo_attributes[i] != nullptr; i++) {
                char *string = g_file_info_get_attribute_as_string(gInfo, metainfo_attributes[i]);
                if (string) {
                    auto var = QVariant(string);
                    this->setMetaInfoVariant(metainfo_attributes[i], var);
                    g_free(string);
                }
            }
            g_strfreev(metainfo_attributes);
        }
    }
}

void FileMetaInfo::setMetaInfoInt(const QString &key, int value)
{
    setMetaInfoVariant(key, QString::number(value));
}

void FileMetaInfo::setMetaInfoString(const QString &key, const QString &value)
{
    setMetaInfoVariant(key, value);;
}

void FileMetaInfo::setMetaInfoStringList(const QString &key, const QStringList &value)
{
    QString string = value.join('\n');
    setMetaInfoVariant(key, string);
}

void FileMetaInfo::setMetaInfoVariant(const QString &key, const QVariant &value)
{
    if (!mMutex.tryLock(300)) {
        return;
    }

    GError *err = nullptr;
    QString realKey = key;

    if (!key.startsWith("metadata::")) {
        realKey = "metadata::" + key;
    }

    mMetaHash.remove(realKey);
    mMetaHash.insert(realKey, value);
    GFile *file = g_file_new_for_uri(mUri.toUtf8().constData());
    GFileInfo *info = g_file_info_new();
    std::string tmp = realKey.toStdString();
    auto data = value.toString().toUtf8().data();
    g_file_info_set_attribute(info, tmp.c_str(), G_FILE_ATTRIBUTE_TYPE_STRING, (gpointer)data);

    g_file_set_attribute(file, tmp.c_str(), G_FILE_ATTRIBUTE_TYPE_STRING, (gpointer)data, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, nullptr, &err);
    if (err) {
        g_error_free(err);
    }

    g_object_unref(info);
    g_object_unref(file);
    mMutex.unlock();
}

const QVariant FileMetaInfo::getMetaInfoVariant(const QString &key)
{
    QString realKey = key;

    if (!key.startsWith("metadata::")) {
        realKey = "metadata::" + key;
    }

    if (mMetaHash.value(realKey).isValid()) {
        return mMetaHash.value(realKey);
    }

    return QVariant();
}

const QString FileMetaInfo::getMetaInfoString(const QString &key)
{
    return getMetaInfoVariant(key).toString();
}

const QStringList FileMetaInfo::getMetaInfoStringList(const QString &key)
{
    return getMetaInfoVariant(key).toString().split('\n');
}

int FileMetaInfo::getMetaInfoInt(const QString &key)
{
    return getMetaInfoVariant(key).toString().toInt();
}

void FileMetaInfo::removeMetaInfo(const QString &key)
{
    if (!mMutex.tryLock(300)) {
        return;
    }

    QString realKey = key;
    if (!key.startsWith("metadata::")) {
        realKey = "metadata::" + key;
    }

    mMetaHash.remove(realKey);
    GFile *file = g_file_new_for_uri(mUri.toUtf8().constData());
    g_file_set_attribute(file, realKey.toUtf8().constData(), G_FILE_ATTRIBUTE_TYPE_INVALID, nullptr, G_FILE_QUERY_INFO_NONE, nullptr, nullptr);

    g_object_unref(file);
    mMutex.unlock();
}
