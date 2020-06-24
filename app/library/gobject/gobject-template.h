#ifndef GOBJECTTEMPLATE_H
#define GOBJECTTEMPLATE_H

#include <memory>
#include <gio/gio.h>
#include <glib-object.h>

template<class T>

class GobjectTemplate
{
public:
    GobjectTemplate ();
    GobjectTemplate (T* obj, bool ref = false) {
        mObj = obj;
        if (ref) {
            g_object_ref(obj);
        }
    }
    ~GobjectTemplate () {
        if (mObj) {
            g_object_unref(mObj);
        }
    }

    T* get () {
        return mObj;
    }

private:
    mutable T* mObj = nullptr;
};

typedef std::shared_ptr<GobjectTemplate<GIcon>> GIconWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GFile>> GFileWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GDrive>> GDriveWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GMount>> GMountWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GVolume>> GVolumeWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GFileInfo>> GFileInfoWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GThemedIcon>> GThemedIconWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GCancellable>> GCancellableWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GFileMonitor>> GFileMonitorWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GVolumeMonitor>> GVolumeMonitorWrapperPtr;
typedef std::shared_ptr<GobjectTemplate<GFileEnumerator>> GFileEnumeratorWrapperPtr;


std::shared_ptr<GobjectTemplate<GIcon>> wrapGIcon(GIcon *icon);
std::shared_ptr<GobjectTemplate<GFile>> wrapGFile(GFile *file);
std::shared_ptr<GobjectTemplate<GDrive>> wrapGDrive(GDrive *drive);
std::shared_ptr<GobjectTemplate<GMount>> wrapGMount(GMount *mount);
std::shared_ptr<GobjectTemplate<GVolume>> wrapGVolume(GVolume *volume);
std::shared_ptr<GobjectTemplate<GFileInfo>> wrapGFileInfo(GFileInfo *info);
std::shared_ptr<GobjectTemplate<GThemedIcon>> wrapGThemedIcon(GThemedIcon *icon);
std::shared_ptr<GobjectTemplate<GFileMonitor>> wrapGFileMonitor(GFileMonitor *monitor);
std::shared_ptr<GobjectTemplate<GCancellable>> wrapGCancellable(GCancellable *cancellable);
std::shared_ptr<GobjectTemplate<GVolumeMonitor>> wrapGVolumeMonitor(GVolumeMonitor *monitor);
std::shared_ptr<GobjectTemplate<GFileEnumerator>> wrapGFileEnumerator(GFileEnumerator *enumerator);


#endif // GOBJECTTEMPLATE_H
