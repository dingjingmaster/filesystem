#include "search-vfs-file.h"

#include "search-vfs-manager.h"
#include "search-vfs-file-enumerator.h"

#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <file-enumerator.h>

static void file_dispose(GObject *object);
GFile* fm_search_vfs_file_dup(GFile *file);
char *fm_search_vfs_file_get_uri(GFile *file);
char *fm_search_vfs_file_get_path(GFile *file);
GFile *fm_search_vfs_file_get_parent(GFile *file);
gboolean fm_search_vfs_file_is_native(GFile *file);
GFile* fm_search_vfs_file_new_for_uri(const char *uri);
static void fm_search_vfs_file_init(FmSearchVFSFile *self);
static void fm_search_vfs_file_g_file_iface_init(GFileIface *iface);
static void fm_search_vfs_file_class_init (FmSearchVFSFileClass *klass);
GFile* fm_search_vfs_file_resolve_relative_path(GFile *file, const char *relative_path);
void fm_search_vfs_file_enumerator_parse_uri(FmSearchVFSFileEnumerator *enumerator, const char *uri);
GFileInfo *fm_search_vfs_file_query_info(GFile *file, const char *attributes, GFileQueryInfoFlags flags, GCancellable *cancellable, GError **error);
GFileEnumerator *fm_search_vfs_file_enumerate_children(GFile *file, const char *attribute, GFileQueryInfoFlags flags, GCancellable *cancellable, GError **error);
GFileEnumerator *fm_search_vfs_file_enumerate_children_internal(GFile *file, const char *attributes, GFileQueryInfoFlags flags, GCancellable *cancellable, GError **error);

G_DEFINE_TYPE_EXTENDED(FmSearchVFSFile, fm_search_vfs_file, G_TYPE_OBJECT, 0, G_ADD_PRIVATE(FmSearchVFSFile) G_IMPLEMENT_INTERFACE(G_TYPE_FILE, fm_search_vfs_file_g_file_iface_init));


static void file_dispose(GObject *object)
{
    auto vfs_file = FM_SEARCH_VFS_FILE(object);
    if (vfs_file->priv->uri) {
        g_free(vfs_file->priv->uri);
    }
}

GFile* fm_search_vfs_file_dup(GFile *file)
{
    if (!FM_IS_SEARCH_VFS_FILE(file)) {
        //this should not be happend.
        return g_file_new_for_uri("search:///");
    }
    auto vfs_file = FM_SEARCH_VFS_FILE(file);
    auto dup = FM_SEARCH_VFS_FILE(g_object_new(FM_TYPE_SEARCH_VFS_FILE, nullptr));
    dup->priv->uri = g_strdup(vfs_file->priv->uri);
    return G_FILE(dup);
}

char *fm_search_vfs_file_get_uri(GFile *file)
{
    auto vfs_file = FM_SEARCH_VFS_FILE(file);
    return g_strdup(vfs_file->priv->uri);
}

char *fm_search_vfs_file_get_path(GFile *file)
{
    return nullptr;
}

GFile *fm_search_vfs_file_get_parent(GFile *file)
{
    Q_UNUSED(file);
    return nullptr;
}

gboolean fm_search_vfs_file_is_native(GFile *file)
{
    return false;
}

GFile* fm_search_vfs_file_new_for_uri(const char *uri)
{
    auto search_vfs_file = FM_SEARCH_VFS_FILE(g_object_new(FM_TYPE_SEARCH_VFS_FILE, nullptr));
    search_vfs_file->priv->uri = g_strdup(uri);

    return G_FILE(search_vfs_file);
}

static void fm_search_vfs_file_init(FmSearchVFSFile *self)
{
    FmSearchVFSFilePrivate *priv = (FmSearchVFSFilePrivate*) fm_search_vfs_file_get_instance_private(self);
    self->priv = priv;
    priv->uri = nullptr;
}

static void fm_search_vfs_file_g_file_iface_init(GFileIface *iface)
{
    iface->dup = fm_search_vfs_file_dup;
    iface->get_parent = fm_search_vfs_file_get_parent;
    iface->is_native = fm_search_vfs_file_is_native;
    iface->enumerate_children = fm_search_vfs_file_enumerate_children;
    iface->query_info = fm_search_vfs_file_query_info;
    iface->get_uri = fm_search_vfs_file_get_uri;
    iface->get_path = fm_search_vfs_file_get_path;
    iface->resolve_relative_path = fm_search_vfs_file_resolve_relative_path;
}

static void fm_search_vfs_file_class_init (FmSearchVFSFileClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = file_dispose;
}

GFile* fm_search_vfs_file_resolve_relative_path(GFile *file, const char *relative_path)
{
    Q_UNUSED(file);
    //FIXME: maybe i should put the resolve to vfs look up method?
    QString tmp = relative_path;
    if (tmp.contains("real-uri:")) {
        tmp = tmp.remove("real-uri:");
        return g_file_new_for_uri(tmp.toUtf8().constData());
    }
    return g_file_new_for_uri("serarch:///");
}

void fm_search_vfs_file_enumerator_parse_uri(FmSearchVFSFileEnumerator *enumerator, const char *uri)
{
    FmSearchVFSFileEnumeratorPrivate *details = enumerator->priv;

    *details->searchVfsDirectoryUri = uri;

    auto manager = SearchVFSManager::getInstance();
    if (manager->hasHistory(uri)) {
        auto uris = manager->getHistroyResults(uri);
        for (auto uri: uris) {
            details->enumerateQueue->enqueue(uri);
        }
        //do not parse uri, not neccersary
        return;
    }

    QStringList args = details->searchVfsDirectoryUri->split("&", QString::SkipEmptyParts);

    //we should judge case sensitive, then we confirm the regexp when
    //we match file in file enumeration.
    for (auto arg: args) {
        //qDebug()<<arg;
        if (arg.contains("search_hidden=")) {
            if (arg.endsWith("1")) {
                details->searchHidden = true;
            }
            continue;
        }
        if (arg.contains("use_regexp=")) {
            if (arg.endsWith("0")) {
                details->useRegexp = false;
            }
            continue;
        }

        if (arg.contains("case_sensitive=")) {
            if (arg.endsWith("1")) {
                details->caseSensitive = false;
            }
            continue;
        }

        if (arg.contains("name_regexp=")) {
            if (arg == "name_regexp=") {
                continue;
            }
            QString tmp = arg;
            tmp = tmp.remove("name_regexp=");
            details->nameRegexp = new QRegExp(tmp);
            continue;
        }

        if (arg.contains("extend_regexp="))
        {
            if (arg == "extend_regexp=")
                continue;

            QString tmp = arg;
            tmp = tmp.remove("extend_regexp=");
            QStringList keys = tmp.split(",", QString::SkipEmptyParts);
            for(auto key : keys)
            {
                details->nameRegexpExtendList->append(new QRegExp(key));
            }
            continue;
        }

        if (arg.contains("content_regexp=")) {
            if (arg == "content_regexp=") {
                continue;
            }
            QString tmp = arg;
            tmp = tmp.remove("content_regexp=");
            details->contentRegexp = new QRegExp(tmp);
            continue;
        }

        if (arg.contains("save=")) {
            if (arg.endsWith("1")) {
                details->saveResult = true;
            }
            continue;
        }

        if (arg.contains("recursive=")) {
            if (arg.endsWith("1")) {
                details->recursive = true;
            }
            else
                details->saveResult = false;
            continue;
        }

        if (arg.contains("search_uris=")) {
            QString tmp = arg;
            tmp.remove("search:///");
            tmp.remove("search_uris=");
            QStringList uris = tmp.split(",", QString::SkipEmptyParts);
            for (auto uri: uris) {
                //NOTE: we should enumerate the search uris and add
                //the children into queue first. otherwise we could
                //not judge wether we should search recursively.
                FileEnumerator e;
                e.setEnumerateDirectory(uri);
                e.enumerateSync();
                auto uris1 = e.getChildrenUris();
                for (auto uri1 : uris1) {
                    details->enumerateQueue->enqueue(uri1);
                }
            }
        }
    }

    Qt::CaseSensitivity sensitivity = details->caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if (!details->nameRegexp) {
        //details->name_regexp = new QRegExp;
    } else {
        details->nameRegexp->setCaseSensitivity(sensitivity);
    }

    if (!details->contentRegexp) {
        //details->content_regexp = new QRegExp;
    } else {
        details->contentRegexp->setCaseSensitivity(sensitivity);
    }

    if (details->nameRegexpExtendList->count() >0)
    {
        for(int i=0; i<details->nameRegexpExtendList->count(); i++)
        {
            details->nameRegexpExtendList->at(i)->setCaseSensitivity(sensitivity);
        }
    }
}

GFileInfo *fm_search_vfs_file_query_info(GFile *file, const char *attributes, GFileQueryInfoFlags flags, GCancellable *cancellable, GError **error)
{
    auto vfs_file = FM_SEARCH_VFS_FILE(file);
    GFileInfo *info = g_file_info_new();
    g_file_info_set_name(info, vfs_file->priv->uri);
    auto icon = g_themed_icon_new("search");
    g_file_info_set_icon(info, icon);
    g_object_unref(icon);
    g_file_info_set_display_name(info, "");
    g_file_info_set_file_type(info, G_FILE_TYPE_DIRECTORY);
    return info;
}

GFileEnumerator *fm_search_vfs_file_enumerate_children(GFile *file, const char *attribute, GFileQueryInfoFlags flags, GCancellable *cancellable, GError **error)
{
    auto searchVfsFile = FM_SEARCH_VFS_FILE(file);

    return fm_search_vfs_file_enumerate_children_internal(file, attribute, flags, cancellable, error);
}

GFileEnumerator* fm_search_vfs_file_enumerate_children_internal(GFile *file, const char *attributes, GFileQueryInfoFlags flags, GCancellable *cancellable, GError **error)
{
    auto vfs_file = FM_SEARCH_VFS_FILE(file);

    //we should add enumerator container when search vfs enumerator created.
    //otherwise g_enumerator_get_child will went error.
    auto enumerator = FM_SEARCH_VFS_FILE_ENUMERATOR(g_object_new(FM_TYPE_SEARCH_VFS_FILE_ENUMERATOR, "container", file, nullptr));
    fm_search_vfs_file_enumerator_parse_uri(enumerator, vfs_file->priv->uri);
    //parse uri, add top folder uri to queue;
    return G_FILE_ENUMERATOR(enumerator);
}
