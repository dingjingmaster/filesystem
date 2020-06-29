#ifndef FILEENUMERATOR_H
#define FILEENUMERATOR_H

#include <memory>
#include <QObject>

#include <gio/gio.h>
#include <gobject/gerror-wrapper.h>

class FileInfo;

class FileEnumerator : public QObject
{
    Q_OBJECT
public:
    explicit FileEnumerator (QObject *parent = nullptr);
    ~ FileEnumerator ();

    void prepare ();
    void enumerateSync ();
    const QStringList getChildrenUris ();
    void setEnumerateDirectory (QString uri);
    void setEnumerateDirectory (GFile *file);
    void setAutoDelete (bool autoDelete = true);
    const QList<std::shared_ptr<FileInfo>> getChildren (bool addToHash = false);

Q_SIGNALS:
    void enumerateFinished (bool successed = false);
    void childrenUpdated (const QStringList &uriList);
    void prepared (const std::shared_ptr<GerrorWrapper> &err = nullptr, const QString &targetUri = nullptr, bool critical = false);

public Q_SLOTS:
    void slotCancel ();
    void slotEnumerateAsync ();

protected:
    GFile *enumerateTargetFile ();
    void handleError (GError *err);
    void enumerateChildren (GFileEnumerator *enumerator);
    static GAsyncReadyCallback mountMountableCallback (GFile *file, GAsyncResult *res, FileEnumerator *pThis);
    static GAsyncReadyCallback mountEnclosingVolume_callback (GFile *file, GAsyncResult *res, FileEnumerator *pThis);
    static GAsyncReadyCallback findChildrenAsyncReadyCallback (GFile *file, GAsyncResult *res, FileEnumerator *pThis);
    static GAsyncReadyCallback enumeratorNextFilesAsyncReadyCallback (GFileEnumerator *enumerator, GAsyncResult *res, FileEnumerator *pThis);

private:
    bool mAutoDelete = false;
    GFile *mRootFile = nullptr;
    GCancellable *mCancellable = nullptr;
    QList<QString> *mChildrenUris = nullptr;


};

#endif // FILEENUMERATOR_H
