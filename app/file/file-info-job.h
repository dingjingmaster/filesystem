#ifndef FILEINFOJOB_H
#define FILEINFOJOB_H

#include <memory>
#include <QObject>
#include <gio/gio.h>

class FileInfo;

class FileInfoJob : public QObject
{
    friend class FileInfo;
    Q_OBJECT
public:
    explicit FileInfoJob (std::shared_ptr<FileInfo> info, QObject *parent = nullptr);
    explicit FileInfoJob (const QString &uri, QObject *parent = nullptr);

    ~FileInfoJob ();
    bool querySync ();
    std::shared_ptr<FileInfo> getInfo ();

    void setAutoDelete (bool deleteWhenJobFinished = true);

Q_SIGNALS:
    void infoUpdated ();
    void queryAsyncFinished (bool successed);

public Q_SLOTS:
    void cancel ();
    void queryAsync ();
    QString getAppName(QString desktopfp);

protected:
    static GAsyncReadyCallback queryInfoAsyncCallback(GFile *file, GAsyncResult *res, FileInfoJob *thisJob);

private:
    void refreshInfoContents(GFileInfo *newInfo);

private:
    bool mAutoDelete = false;
    std::shared_ptr<FileInfo> mInfo;

};

#endif // FILEINFOJOB_H
