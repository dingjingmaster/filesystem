#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <gio/gio.h>

#include <QObject>

class FileWatcher : public QObject
{
    Q_OBJECT
public:
    FileWatcher (QString uri = nullptr, QObject* parent = nullptr);
    ~FileWatcher ();

    void stopMonitor ();
    void startMonitor ();
    bool supportMonitor ();
    const QString currentUri ();
    void setMonitorChildrenChange (bool monitorChildrenChange = true);

Q_SIGNALS:
    void requestUpdateDirectory ();
    void fileCreated (const QString& uri);
    void fileDeleted (const QString& uri);
    void fileChanged (const QString& uri);
    void directoryDeleted (const QString& uri);
    void thumbnailUpdated (const QString& uri);
    void directoryUnmounted (const QString& uri);
    void locationChanged (const QString& oldUri, const QString& newUri);

public Q_SLOTS:
    void slotCancel ();

protected:
    void prepare ();
    void changeMonitorUri(QString uri);
    static void dir_changed_callback(GFileMonitor *monitor, GFile *file, GFile *otherFile, GFileMonitorEvent eventType, FileWatcher *pThis);
    static void file_changed_callback(GFileMonitor *monitor, GFile *file, GFile *otherFile, GFileMonitorEvent eventType, FileWatcher *pThis);

private:

    bool                mSupportMonitor = true;
    bool                mMontorChildrenChange = false;

    QString             mUri = nullptr;
    GFile*              mFile = nullptr;
    QString             mTargetUri = nullptr;
    GFileMonitor*       mMonitor = nullptr;
    GFileMonitor*       mDirMonitor = nullptr;

    gulong              mDirHandle = 0;
    gulong              mFileHandle = 0;

    GError*             mMonitorErr = nullptr;
    GError*             mDirMonitorErr = nullptr;
    GCancellable*       mCancellable = nullptr;
};

#endif // FILEWATCHER_H
