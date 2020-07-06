#ifndef THUMBNAILJOB_H
#define THUMBNAILJOB_H

#include <memory>
#include <QObject>
#include <QRunnable>

class FileWatcher;

class ThumbnailJob : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ThumbnailJob (const QString &uri, const std::shared_ptr<FileWatcher> watcher, QObject *parent = nullptr);
    ~ThumbnailJob ();

public Q_SLOTS:
    void run () override;

private:
    QString mUri;
    std::weak_ptr<FileWatcher> mWatcher;

};

#endif // THUMBNAILJOB_H
