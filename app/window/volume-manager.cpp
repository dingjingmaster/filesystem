#include "volume-manager.h"


Drive::Drive()
{

}

Drive::Drive(GDrive *drive, bool takeOver)
{
    mDrive = drive;
    mTakeOver = takeOver;
}

Drive::~Drive()
{
    if (mTakeOver) {
        g_object_unref(mDrive);
    }
}

QString Drive::name()
{
    if (!mDrive) {
        return nullptr;
    }

    char *name = g_drive_get_name(mDrive);
    QString value = name;
    g_free(name);

    return value;
}

QString Drive::iconName()
{
    if (!mDrive) {
        return nullptr;
    }

    GThemedIcon *g_icon = G_THEMED_ICON(g_drive_get_icon(mDrive));
    const gchar* const* icon_names = g_themed_icon_get_names(G_THEMED_ICON (g_icon));
    g_object_unref(g_icon);

    return *icon_names;
}

QString Drive::symbolicIconName()
{
    if (!mDrive) {
        return nullptr;
    }

    GThemedIcon *gIcon = G_THEMED_ICON(g_drive_get_symbolic_icon(mDrive));
    const gchar* const* iconNames = g_themed_icon_get_names(G_THEMED_ICON (gIcon));
    g_object_unref(gIcon);

    return *iconNames;
}

bool Drive::removable()
{
    if (!mDrive) {
        return false;
    }
#if GLIB_CHECK_VERSION(2, 50, 0)
    return g_drive_is_removable(mDrive);
#else
    //FIXME: old glib does not have relative api.
    //should i implete it?
    return false;
#endif
}

GDrive *Drive::getGDrive()
{
    return mDrive;
}

Volume::Volume()
{

}

Volume::Volume(GVolume *volume, bool takeOver)
{
    mVolume = volume;
    mTakeOver = takeOver;
}

Volume::~Volume()
{
    if (mTakeOver)
        g_object_unref(mVolume);
}

QString Volume::name()
{
    char *name = g_volume_get_name(mVolume);
    QString value = name;
    g_free(name);
    return value;
}

QString Volume::iconName()
{
    GThemedIcon *g_icon = G_THEMED_ICON(g_volume_get_icon(mVolume));
    const gchar* const* icon_names = g_themed_icon_get_names(G_THEMED_ICON (g_icon));
    g_object_unref(g_icon);
    return *icon_names;
}

QString Volume::symbolicIconName()
{
    GThemedIcon *g_icon = G_THEMED_ICON(g_volume_get_symbolic_icon(mVolume));
    const gchar* const* icon_names = g_themed_icon_get_names(G_THEMED_ICON (g_icon));
    g_object_unref(g_icon);
    return *icon_names;
}

GVolume *Volume::getGVolume()
{
    return mVolume;
}

QString Volume::rootUri()
{
    GFile *root = g_volume_get_activation_root(mVolume);
    if (!G_IS_FILE (root)) {
        return nullptr;
    }
    char *uri = g_file_get_uri(root);
    QString value = uri;
    g_free(uri);
    return value;
}

QString Volume::uuid()
{
    char *uuid = g_volume_get_uuid(mVolume);
    QString value = uuid;
    if (uuid) {
        g_free(uuid);
    }
    return value;
}

Mount::Mount()
{

}

Mount::Mount(GMount *mount, bool takeOver)
{
    mMount = mount;
    mTakeOver = takeOver;
}

Mount::~Mount()
{
    if (mTakeOver) {
        g_object_unref(mMount);
    }
}

QString Mount::name()
{
    char *name = g_mount_get_name(mMount);
    QString value = name;
    g_free(name);
    return value;
}

QString Mount::iconName()
{
    GThemedIcon *gIcon = G_THEMED_ICON(g_mount_get_icon(mMount));
    const gchar* const* iconNames = g_themed_icon_get_names(G_THEMED_ICON (gIcon));
    g_object_unref(gIcon);
    return *iconNames;
}

QString Mount::symbolicIconName()
{
    GThemedIcon *gIcon = G_THEMED_ICON(g_mount_get_symbolic_icon(mMount));
    const gchar* const* iconNames = g_themed_icon_get_names(G_THEMED_ICON (gIcon));
    g_object_unref(gIcon);
    return *iconNames;
}

GMount *Mount::getGMount()
{
    return mMount;
}

QString Mount::rootUri()
{
    GFile *root = g_mount_get_root(mMount);
    char *uri = g_file_get_uri(root);
    QString value = uri;
    g_free(uri);
    return value;
}

QString Mount::uuid()
{
    char *uuid = g_mount_get_uuid(mMount);
    QString value = uuid;
    if (uuid) {
        g_free(uuid);
    }
    return value;
}

VolumeManager *VolumeManager::getInstance()
{

}

std::shared_ptr<Drive> VolumeManager::getDriveFromUri(const QString &uri)
{

}

std::shared_ptr<Mount> VolumeManager::getMountFromUri(const QString &uri)
{

}

std::shared_ptr<Volume> VolumeManager::getVolumeFromUri(const QString &uri)
{

}

std::shared_ptr<Drive> VolumeManager::getDriveFromMount(const std::shared_ptr<Mount> &mount)
{

}

std::shared_ptr<Volume> VolumeManager::getVolumeFromMount(const std::shared_ptr<Mount> &mount)
{

}

void VolumeManager::mountAddedCallback(GVolumeMonitor *monitor, GMount *mount, VolumeManager *p_this)
{

}

void VolumeManager::mountRemovedCallback(GVolumeMonitor *monitor, GMount *mount, VolumeManager *p_this)
{

}

void VolumeManager::volumeAddedCallback(GVolumeMonitor *monitor, GVolume *volume, VolumeManager *p_this)
{

}

void VolumeManager::driveConnectedCallback(GVolumeMonitor *monitor, GDrive *drive, VolumeManager *p_this)
{

}

void VolumeManager::volumeRemovedCallback(GVolumeMonitor *monitor, GVolume *volume, VolumeManager *p_this)
{

}

void VolumeManager::driveDisconnectedCallback(GVolumeMonitor *monitor, GDrive *drive, VolumeManager *p_this)
{

}

void VolumeManager::unmount(const QString &uri)
{

}

void VolumeManager::unmountCb(GFile *file, GAsyncResult *result, GError **error)
{

}

VolumeManager::VolumeManager(QObject *parent) : QObject(parent)
{

}

VolumeManager::~VolumeManager()
{

}
