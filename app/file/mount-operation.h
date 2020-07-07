#ifndef MOUNTOPERATION_H
#define MOUNTOPERATION_H

#include <memory>
#include <QObject>
#include <gio/gio.h>

class GerrorWrapper;

class MountOperation : public QObject
{
    Q_OBJECT
public:
    explicit MountOperation(QString uri, QObject *parent = nullptr);
    ~MountOperation();
    void setAutoDelete(bool isAuto = true);

Q_SIGNALS:
    void cancelled();
    void mountSuccess(QString uri);
    void finished(const std::shared_ptr<GerrorWrapper> &err = nullptr);

public Q_SLOTS:
    void slotStart();
    void slotCancel();

protected:
    static void abortedCB (GMountOperation *op, MountOperation *pThis);
    static void askQuestionCB (GMountOperation *op, char *message, char **choices, MountOperation *pThis);
    static GAsyncReadyCallback mountEnclosingVolumeCallback(GFile *volume, GAsyncResult *res, MountOperation *pThis);
    static void askPasswordCB (GMountOperation *op, const char* message, const char* defaultUser, const char* defaultDomain, GAskPasswordFlags flags, MountOperation *pThis);

private:
    QString remoteUri = "";
    GList *mErrs = nullptr;
    GFile *mVolume = nullptr;
    bool mAutoDelete = false;
    GMountOperation *mOp = nullptr;
    GCancellable *mCancellable = nullptr;
};

#endif // MOUNTOPERATION_H
