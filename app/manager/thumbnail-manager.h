#ifndef THUMBNAILMANAGER_H
#define THUMBNAILMANAGER_H

#include <QHash>
#include <QIcon>
#include <QMutex>
#include <memory>

class QSemaphore;
class QThreadPool;
class FileWatcher;

class ThumbnailManager : public QObject
{
    friend class ThumbnailJob;
    Q_OBJECT

public:
    bool hasThumbnail(const QString &uri);
    static ThumbnailManager *getInstance();
    void releaseThumbnail(const QString &uri);
    void setForbidThumbnailInView(bool forbid);
    const QIcon tryGetThumbnail(const QString &uri);
    void updateDesktopFileThumbnail(const QString &uri, std::shared_ptr<FileWatcher> watcher = nullptr);
    void createThumbnail(const QString &uri, std::shared_ptr<FileWatcher> watcher = nullptr, bool force = false);

public Q_SLOTS:
    void syncThumbnailPreferences();

protected:
    void insertOrUpdateThumbnail(const QString &uri, const QIcon &icon);

private:
    explicit ThumbnailManager(QObject *parent = nullptr);
    ~ThumbnailManager();

    void createThumbnailInternal(const QString &uri, std::shared_ptr<FileWatcher> watcher = nullptr, bool force = false);

private:
    QSemaphore *mSemaphore;
    QHash<QString, QIcon> mHash;
    QThreadPool *mThumbnailThreadPool;
};

#endif // THUMBNAILMANAGER_H
