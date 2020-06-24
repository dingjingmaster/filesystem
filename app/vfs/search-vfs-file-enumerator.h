#ifndef SEARCHVFSFILEENUMERATOR_H
#define SEARCHVFSFILEENUMERATOR_H

//#include "file-info.h"

#include <QQueue>
#include <QRegExp>
#include <gio/gio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FM_TYPE_SEARCH_VFS_FILE_ENUMERATOR fm_search_vfs_file_enumerator_get_type()
G_DECLARE_FINAL_TYPE(FmSearchVFSFileEnumerator, fm_search_vfs_file_enumerator, FM, SEARCH_VFS_FILE_ENUMERATOR, GFileEnumerator)

FmSearchVFSFileEnumerator *fm_search_vfs_file_enumerator_new(void);

typedef struct _FmSearchVFSFileEnumerator FmSearchVFSFileEnumerator;
typedef struct _FmSearchVFSFileEnumeratorPrivate FmSearchVFSFileEnumeratorPrivate;

struct _FmSearchVFSFileEnumeratorPrivate
{
    gboolean            useRegexp;
    gboolean            recursive;
    gboolean            saveResult;
    gboolean            searchHidden;
    gboolean            caseSensitive;
    QRegExp             *nameRegexp;
    QRegExp             *contentRegexp;
    gboolean            matchNameOrContent;
    QQueue<QString>     *enumerateQueue;
    QList<QRegExp*>     *nameRegexpExtendList;
    QString             *searchVfsDirectoryUri;
};

struct _FmSearchVFSFileEnumerator
{
    GFileEnumerator parent_instance;
    FmSearchVFSFileEnumeratorPrivate *priv;
};

#ifdef __cplusplus
}
#endif
#endif // SEARCHVFSFILEENUMERATOR_H
