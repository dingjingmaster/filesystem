#ifndef VOLUMEMANAGER_H
#define VOLUMEMANAGER_H

#include <memory>
#include <QObject>
#include <gio/gio.h>
#include <QMetaType>

class Drive
{
public:
    Drive ();
    Drive (GDrive *drive, bool takeOver = false);
    ~Drive ();

    QString name ();
    bool removable();
    GDrive *getGDrive();
    QString iconName ();
    QString symbolicIconName ();

private:
    GDrive *mDrive = nullptr;
    bool mTakeOver = false;
};


class Volume
{
public:
    Volume ();
    Volume (GVolume *volume, bool takeOver = false);
    ~Volume ();
    QString name ();
    QString uuid ();
    QString rootUri ();
    QString iconName ();
    GVolume *getGVolume ();
    QString symbolicIconName ();

private:
    bool mTakeOver = false;
    GVolume* mVolume = nullptr;
};


class Mount
{
public:
    Mount ();
    Mount (GMount *mount, bool takeOver = false);
    ~Mount ();

    QString name ();
    QString uuid ();
    QString rootUri ();
    QString iconName ();
    GMount *getGMount ();
    QString symbolicIconName ();

private:
    bool mTakeOver = false;
    GMount *mMount = nullptr;
};

class VolumeManager : public QObject
{
    Q_OBJECT
public:
    static VolumeManager *getInstance ();
    static std::shared_ptr<Drive> getDriveFromUri (const QString &uri);
    static std::shared_ptr<Mount> getMountFromUri (const QString &uri);
    static std::shared_ptr<Volume> getVolumeFromUri (const QString &uri);
    static std::shared_ptr<Drive> getDriveFromMount (const std::shared_ptr<Mount> &mount);
    static std::shared_ptr<Volume> getVolumeFromMount (const std::shared_ptr<Mount> &mount);

Q_SIGNALS:
    void mountAdded (const std::shared_ptr<Mount> &mount);
    void mountRemoved (const std::shared_ptr<Mount> &mount);
    void volumeAdded (const std::shared_ptr<Volume> &volume);
    void driveConnected (const std::shared_ptr<Drive> &drive);
    void volumeRemoved (const std::shared_ptr<Volume> &volume);
    void driveDisconnected (const std::shared_ptr<Drive> &drive);

protected:
    static void mountAddedCallback (GVolumeMonitor *monitor, GMount *mount, VolumeManager *p_this);
    static void mountRemovedCallback (GVolumeMonitor *monitor, GMount *mount, VolumeManager *p_this);
    static void volumeAddedCallback (GVolumeMonitor *monitor, GVolume *volume, VolumeManager *p_this);
    static void driveConnectedCallback (GVolumeMonitor *monitor, GDrive *drive, VolumeManager *p_this);
    static void volumeRemovedCallback (GVolumeMonitor *monitor, GVolume *volume, VolumeManager *p_this);
    static void driveDisconnectedCallback (GVolumeMonitor *monitor, GDrive *drive, VolumeManager *p_this);

public Q_SLOTS:
    static void unmount (const QString &uri);

protected:
    static void unmountCb (GFile *file, GAsyncResult *result, GError **error);

private:
    explicit VolumeManager (QObject *parent = nullptr);
    ~VolumeManager ();

    gulong mMountAddedHandle = 0;
    gulong mVolumeAddedHandle = 0;
    gulong mMountRemovedHandle = 0;
    gulong mVolumeRemovedHandle = 0;
    gulong mDriveConnectedHandle = 0;
    gulong mDriveDisconnectedHandle = 0;
    GVolumeMonitor *mVolumeMonitor = nullptr;
};

Q_DECLARE_METATYPE(Drive)
Q_DECLARE_METATYPE(Mount)
Q_DECLARE_METATYPE(Volume)

Q_DECLARE_METATYPE(std::shared_ptr<Drive>)
Q_DECLARE_METATYPE(std::shared_ptr<Mount>)
Q_DECLARE_METATYPE(std::shared_ptr<Volume>)


#endif // VOLUMEMANAGER_H
