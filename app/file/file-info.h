#ifndef FILEINFO_H
#define FILEINFO_H

#include <memory>
#include <QMutex>
#include <QObject>

#include <gio/gio.h>

class FileInfoJob;
class FileMetaInfo;

class FileInfo : public QObject
{
    friend class FileInfoJob;
    friend class FileMetaInfo;

    Q_OBJECT
public:
    enum Access
    {
        Readable,
        Writeable,
        Executable,
        Deleteable,
        Trashable,
        Renameable
    };
    Q_DECLARE_FLAGS(AccessFlags, Access);

    explicit FileInfo(QObject *parent = nullptr);
    explicit FileInfo(const QString &uri, QObject *parent = nullptr);
    ~FileInfo();
    static std::shared_ptr<FileInfo> fromUri(QString uri, bool addToHash = true);
    static std::shared_ptr<FileInfo> fromPath(QString path, bool addToHash = true);
    static std::shared_ptr<FileInfo> fromGFile(GFile *file, bool addToHash = true);

    bool isDir();
    QString uri();
    quint64 size();
    QString type();
    bool isVolume();
    bool isVirtual();
    QString fileID();
    QString iconName();
    QString mimeType();
    QString fileType();
    QString fileSize();
    bool isSymbolLink();
    quint64 accessTime();
    QString accessDate();
    QString displayName();
    quint64 modifiedTime();
    QString modifiedDate();
    QString symbolicIconName();

    bool canRead();
    bool canWrite();
    bool canTrash();
    bool canDelete();
    bool canRename();
    bool canExecute();

    bool canMount();
    bool canUnmount();
    bool canEject();
    bool canStart();
    bool canStop();

    bool isEmptyInfo();
    bool isDesktopFile();
    GFile *gFileHandle();
    AccessFlags accesses();

    //const QIcon thumbnail() {return m_thumbnail;}
    //void setThumbnail(const QIcon &thumbnail) {m_thumbnail = thumbnail;}

Q_SIGNALS:
    void updated();

private:
    QString mUri = nullptr;

    bool mIsDir = false;
    bool mIsValid = false;
    bool mIsVolume = false;
    bool mIsRemote = false;
    bool mIsLoaded = false;
    bool mIsVirtual = false;
    bool mIsSymbolLink = false;

    QString mFileId = nullptr;
    QString mIconName = nullptr;
    QString mDisplayName = nullptr;
    QString mSymbolicIconName = nullptr;
    guint64 mSize = 0;
    guint64 mAccessTime = 0;
    guint64 mModifiedTime = 0;
    QString mContentType = nullptr;

    QString mFileType = nullptr;
    QString mFileSize = nullptr;
    QString mAccessDate = nullptr;
    QString mModifiedDate = nullptr;
    QString mMimeTypeString = nullptr;

    bool mCanRead = true;
    bool mCanStop = false;
    bool mCanWrite = false;
    bool mCanTrash = false;
    bool mCanMount = false;
    bool mCanEject = false;
    bool mCanStart = false;
    GFile *mFile = nullptr;
    bool mCanRename = false;
    bool mCanExcute = false;
    bool mCanDelete = false;
    GFile *mParent = nullptr;
    bool mCanUnmount = false;
    GFile *mTargetFile = nullptr;
    GCancellable *mCancellable = nullptr;
    std::shared_ptr<FileMetaInfo> mMetaInfo = nullptr;

    QMutex mMutex;
};

#endif // FILEINFO_H
