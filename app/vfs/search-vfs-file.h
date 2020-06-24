#ifndef SEARCHVFSFILE_H
#define SEARCHVFSFILE_H

#include <gio/gio.h>
#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FM_TYPE_SEARCH_VFS_FILE fm_search_vfs_file_get_type()

G_DECLARE_FINAL_TYPE(FmSearchVFSFile, fm_search_vfs_file, FM, SEARCH_VFS_FILE, GObject)

FmSearchVFSFile* fm_search_vfs_file_new (void);

typedef struct _FmSearchVFSFile FmSearchVFSFile;
typedef struct _FmSearchVFSFilePrivate FmSearchVFSFilePrivate;

struct _FmSearchVFSFilePrivate
{
    gchar* uri;
};

struct _FmSearchVFSFile
{
    GObject parent_instance;
    FmSearchVFSFilePrivate* priv;
};

GFile* fm_search_vfs_file_new_for_uri (const char* uri);
static GFileEnumerator* fm_search_vfs_file_enumerate_children_internal (GFile* file, const char* attribute, GFileQueryInfoFlags flags, GCancellable* cancellable, GError** error);

#ifdef __cplusplus
}
#endif
#endif // SEARCHVFSFILE_H
