#include "search-vfs-manager.h"
#include "search-vfs-register.h"
#include "search-vfs-file.h"

#include <glib.h>
#include <gio/gio.h>
#include <clib_syslog.h>

#include <QString>
#include <QStringList>

static GFile* test_vfs_lookup (GVfs* vfs, const char *uri, gpointer udata);
static GFile* test_vfs_parse_name (GVfs* vfs, const char *parseName, gpointer udata);

bool isregisted = false;


SearchVFSRegister::SearchVFSRegister()
{

}

void SearchVFSRegister::registSearchVFS()
{
    if (isregisted) return;

    SearchVFSManager::getInstance();

    GVfs *vfs;
    const gchar * const *schemes;
    gboolean res;

    vfs = g_vfs_get_default ();
    schemes = g_vfs_get_supported_uri_schemes(vfs);
    const gchar * const *p;
    p = schemes;
    while (*p) {
        CT_SYSLOG(LOG_DEBUG, "schema: %s", *p)
        p++;
    }

#if GLIB_CHECK_VERSION(2, 50, 0)
    res = g_vfs_register_uri_scheme (vfs, "search", test_vfs_lookup, NULL, NULL, test_vfs_parse_name, NULL, NULL);
#else
    //FIXME: how to implement search operation in old glib?
#endif
}


static GFile* test_vfs_lookup (GVfs* vfs, const char *uri, gpointer udata)
{
    return test_vfs_parse_name(vfs, uri, udata);
}


static GFile* test_vfs_parse_name (GVfs* vfs, const char *parseName, gpointer udata)
{
    QString tmp = parseName;

    if (tmp.contains("real-uri:")) {
        QString realUri = tmp.split("real-uri:").last();
        return g_file_new_for_uri(realUri.toUtf8().constData());
    }
    return fm_search_vfs_file_new_for_uri(parseName);
}
