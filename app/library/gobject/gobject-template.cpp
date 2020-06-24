#include "gobject-template.h"

std::shared_ptr<GobjectTemplate<GFile>> wrapGFile(GFile *file)
{
    return std::make_shared<GobjectTemplate<GFile>>(file);
}

std::shared_ptr<GobjectTemplate<GFileInfo>> wrapGFileInfo(GFileInfo *info)
{
    return std::make_shared<GobjectTemplate<GFileInfo>>(info);
}

std::shared_ptr<GobjectTemplate<GFileEnumerator>> wrapGFileEnumerator(GFileEnumerator *enumerator)
{
    return std::make_shared<GobjectTemplate<GFileEnumerator>>(enumerator);
}

std::shared_ptr<GobjectTemplate<GFileMonitor>> wrapGFileMonitor(GFileMonitor *monitor)
{
    return std::make_shared<GobjectTemplate<GFileMonitor>>(monitor);
}

std::shared_ptr<GobjectTemplate<GVolumeMonitor>> wrapGVolumeMonitor(GVolumeMonitor *monitor)
{
    return std::make_shared<GobjectTemplate<GVolumeMonitor>>(monitor);
}

std::shared_ptr<GobjectTemplate<GDrive>> wrapGDrive(GDrive *drive)
{
    return std::make_shared<GobjectTemplate<GDrive>>(drive);
}

std::shared_ptr<GobjectTemplate<GVolume>> wrapGVolume(GVolume *volume)
{
    return std::make_shared<GobjectTemplate<GVolume>>(volume);
}

std::shared_ptr<GobjectTemplate<GMount>> wrapGMount(GMount *mount)
{
    return std::make_shared<GobjectTemplate<GMount>>(mount);
}

std::shared_ptr<GobjectTemplate<GIcon>> wrapGIcon(GIcon *icon)
{
    return std::make_shared<GobjectTemplate<GIcon>>(icon);
}

std::shared_ptr<GobjectTemplate<GThemedIcon>> wrapGThemedIcon(GThemedIcon *icon)
{
    return std::make_shared<GobjectTemplate<GThemedIcon>>(icon);
}

std::shared_ptr<GobjectTemplate<GCancellable>> wrapGCancellable(GCancellable *cancellable)
{
    return std::make_shared<GobjectTemplate<GCancellable>>(cancellable);
}
