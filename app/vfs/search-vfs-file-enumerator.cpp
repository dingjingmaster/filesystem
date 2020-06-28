#include "search-vfs-file-enumerator.h"
#include "search-vfs-manager.h"

#include <QUrl>
#include <QFile>
#include <QTextStream>

void enumerator_dispose(GObject *object);
static void next_async_op_free (GList *files);
static void fm_search_vfs_file_enumerator_init(FmSearchVFSFileEnumerator *self);
static void fm_search_vfs_file_enumerator_class_init (FmSearchVFSFileEnumeratorClass *klass);
static gboolean enumerator_close(GFileEnumerator *enumerator, GCancellable *cancellable, GError **error);
static GFileInfo *enumerate_next_file(GFileEnumerator *enumerator, GCancellable *cancellable, GError **error);
static GList* enumerate_next_files_finished(GFileEnumerator *enumerator, GAsyncResult *result, GError **error);
gboolean fm_search_vfs_file_enumerator_is_file_match(FmSearchVFSFileEnumerator *enumerator, const QString &uri);
static void next_files_thread (GTask* task, gpointer sourceObject, gpointer taskData, GCancellable *cancellable);
static void fm_search_vfs_file_enumerator_add_directory_to_queue(FmSearchVFSFileEnumerator *enumerator, const QString &directoryUri);
static void enumerate_next_files_async (GFileEnumerator *enumerator, int numFiles, int ioPriority, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer udata);

G_DEFINE_TYPE_WITH_PRIVATE(FmSearchVFSFileEnumerator, fm_search_vfs_file_enumerator, G_TYPE_FILE_ENUMERATOR)


void enumerator_dispose(GObject *object)
{
    FmSearchVFSFileEnumerator *self = FM_SEARCH_VFS_FILE_ENUMERATOR(object);

    if (self->priv->nameRegexp) {
        delete self->priv->nameRegexp;
    }

    if (self->priv->contentRegexp) {
        delete self->priv->contentRegexp;
    }

    delete self->priv->searchVfsDirectoryUri;
    self->priv->enumerateQueue->clear();
    delete self->priv->enumerateQueue;
    for(int i=self->priv->nameRegexpExtendList->count()-1; i>=0; i--) {
        delete self->priv->nameRegexpExtendList->at(i);
    }
    delete self->priv->nameRegexpExtendList;
}

static void next_async_op_free (GList *files)
{
    g_list_free_full (files, g_object_unref);
}

static void fm_search_vfs_file_enumerator_init(FmSearchVFSFileEnumerator *self)
{
    FmSearchVFSFileEnumeratorPrivate *priv = (FmSearchVFSFileEnumeratorPrivate*) fm_search_vfs_file_enumerator_get_instance_private(self);
    self->priv = priv;

    self->priv->searchVfsDirectoryUri = new QString;
    self->priv->enumerateQueue = new QQueue<QString>;
    self->priv->nameRegexpExtendList = new QList<QRegExp*>;
    self->priv->recursive = false;
    self->priv->saveResult = false;
    self->priv->searchHidden = true;
    self->priv->useRegexp = true;
    self->priv->caseSensitive = true;
    self->priv->matchNameOrContent = true;
}

static void fm_search_vfs_file_enumerator_class_init (FmSearchVFSFileEnumeratorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GFileEnumeratorClass *enumerator_class = G_FILE_ENUMERATOR_CLASS(klass);

    gobject_class->dispose = enumerator_dispose;

    enumerator_class->next_file = enumerate_next_file;

    enumerator_class->next_files_async = enumerate_next_files_async;
    enumerator_class->next_files_finish = enumerate_next_files_finished;

    enumerator_class->close_fn = enumerator_close;
}

static gboolean enumerator_close(GFileEnumerator *enumerator, GCancellable *cancellable, GError **error)
{
    FmSearchVFSFileEnumerator *self = FM_SEARCH_VFS_FILE_ENUMERATOR(enumerator);

    return true;
}

static GFileInfo *enumerate_next_file(GFileEnumerator *enumerator, GCancellable *cancellable, GError **error)
{
    auto manager = SearchVFSManager::getInstance();

    if (cancellable) {
        if (g_cancellable_is_cancelled(cancellable)) {
            //FIXME: how to add translation here? do i have to use gettext?
            *error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_CANCELLED, "search is cancelled");
            return nullptr;
        }
    }
    auto search_enumerator = FM_SEARCH_VFS_FILE_ENUMERATOR(enumerator);
    auto enumerate_queue = search_enumerator->priv->enumerateQueue;

    if (manager->hasHistory(*search_enumerator->priv->searchVfsDirectoryUri)) {
        while (!enumerate_queue->isEmpty()) {
            auto uri = enumerate_queue->dequeue();
            auto search_vfs_info = g_file_info_new();
            QString realUriSuffix = "real-uri:" + uri;
            g_file_info_set_name(search_vfs_info, realUriSuffix.toUtf8().constData());
            return search_vfs_info;
        }
        return nullptr;
    }

    while (!enumerate_queue->isEmpty()) {
        //BFS enumeration
        auto uri = enumerate_queue->dequeue();
        GFile *tmp = g_file_new_for_uri(uri.toUtf8().constData());
        GFileType type = g_file_query_file_type(tmp, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, nullptr);
        g_object_unref(tmp);
        bool isDir = type == G_FILE_TYPE_DIRECTORY;
        if (isDir && search_enumerator->priv->recursive) {
            if (!search_enumerator->priv->searchHidden) {
                if (!uri.contains("/.")) {
                    fm_search_vfs_file_enumerator_add_directory_to_queue(search_enumerator, uri);
                }
            } else {
                fm_search_vfs_file_enumerator_add_directory_to_queue(search_enumerator, uri);
            }
        }
        //match
        if (fm_search_vfs_file_enumerator_is_file_match(search_enumerator, uri)) {
            //return this info, and the enumerate get child will return the
            //file crosponding the real uri, due to it would be handled in
            //vfs looking up method callback in registed vfs.
            if (!search_enumerator->priv->searchHidden) {
                if (uri.contains("/.")) {
                    goto return_info;
                }
            } else {
return_info:
                auto search_vfs_info = g_file_info_new();
                QString realUriSuffix = "real-uri:" + uri;
                g_file_info_set_name(search_vfs_info, realUriSuffix.toUtf8().constData());

                if (search_enumerator->priv->saveResult) {
                    auto historyResults = manager->getHistroyResults(*search_enumerator->priv->searchVfsDirectoryUri);
                    //FIXME: add lock?
                    historyResults<<realUriSuffix;
                }
                return search_vfs_info;
            }
        }
    }

    return nullptr;
}

static GList* enumerate_next_files_finished(GFileEnumerator *enumerator, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (result, enumerator), NULL);

    return (GList*)g_task_propagate_pointer (G_TASK (result), error);
}

gboolean fm_search_vfs_file_enumerator_is_file_match(FmSearchVFSFileEnumerator *enumerator, const QString &uri)
{
    FmSearchVFSFileEnumeratorPrivate *details = enumerator->priv;
    if (!details->nameRegexp && !details->contentRegexp && details->nameRegexpExtendList->count() == 0) {
        return false;
    }

    GFile *file = g_file_new_for_uri(uri.toUtf8().constData());
    GFileInfo *info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, nullptr, nullptr);
    g_object_unref(file);

    char *file_display_name = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
    g_object_unref(info);
    QString displayName = file_display_name;
    g_free(file_display_name);
    if (details->nameRegexp) {
        if (details->useRegexp && details->matchNameOrContent && displayName.contains(*enumerator->priv->nameRegexp)) {
            return true;
        } else if (displayName == details->nameRegexp->pattern()) {
            //this is most used for querying files which might be duplicate.
            return true;
        }
    }

    //extend list name match
    if (details->nameRegexpExtendList->count() >0) {
        for(int i=0; i<details->nameRegexpExtendList->count(); i++) {
            auto curRegexp = details->nameRegexpExtendList->at(i);
            //qDebug() <<"curRegexp:" <<*curRegexp<<i;
            if (! curRegexp) {
                continue;
            }

            if (details->useRegexp && details->matchNameOrContent && displayName.contains(*curRegexp)) {
                return true;
            } else if (displayName == curRegexp->pattern()) {
                //this is most used for querying files which might be duplicate.
                return true;
            }
        }
    }

    if (details->contentRegexp) {
        //read file stream
        QUrl url = uri;
        QFile file(url.path());
        file.open(QIODevice::Text | QIODevice::ReadOnly);
        QTextStream stream(&file);
        bool content_matched = false;
        QString line;
        line = stream.readLine();
        while (!line.isNull()) {
            if (line.contains(*enumerator->priv->contentRegexp)) {
                content_matched = true;
                break;
            }
            line = stream.readLine();
        }
        file.close();

        if (content_matched) {
            if (enumerator->priv->matchNameOrContent) {
                return true;
            }
        }
    }

    //this may never happend.
    return false;
}

static void next_files_thread (GTask* task, gpointer sourceObject, gpointer taskData, GCancellable *cancellable)
{
    auto enumerator = G_FILE_ENUMERATOR(sourceObject);
    int num_files = GPOINTER_TO_INT (taskData);
    GFileEnumeratorClass *c;
    GList *files = NULL;
    GError *error = NULL;
    GFileInfo *info;
    int i;

    c = G_FILE_ENUMERATOR_GET_CLASS (enumerator);
    for (i = 0; i < num_files; i++) {
        if (g_cancellable_set_error_if_cancelled (cancellable, &error))
            info = NULL;
        else
            info = c->next_file (enumerator, cancellable, &error);

        if (info == NULL) {
            break;
        } else {
            files = g_list_prepend (files, info);
        }
    }

    if (error) {
        g_task_return_error (task, error);
    } else {
        g_task_return_pointer (task, files, (GDestroyNotify)next_async_op_free);
    }
}

static void enumerate_next_files_async (GFileEnumerator *enumerator, int numFiles, int ioPriority, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer udata)
{
    GTask *task;
    task = g_task_new (enumerator, cancellable, callback, udata);
    g_task_set_source_tag (task, (gpointer)enumerate_next_files_async);
    g_task_set_task_data (task, GINT_TO_POINTER (numFiles), NULL);
    g_task_set_priority (task, ioPriority);

    g_task_run_in_thread (task, next_files_thread);
    g_object_unref (task);
}

static void fm_search_vfs_file_enumerator_add_directory_to_queue(FmSearchVFSFileEnumerator *enumerator, const QString &directoryUri)
{
    auto queue = enumerator->priv->enumerateQueue;

    GError *err = nullptr;
    GFile *top = g_file_new_for_uri(directoryUri.toUtf8().constData());
    GFileEnumerator *e = g_file_enumerate_children(top, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, nullptr, &err);

    if (err) {
        QString errMsg = err->message;
        g_error_free(err);
    }
    g_object_unref(top);
    if (!e)
        return;

    auto child_info = g_file_enumerator_next_file(e, nullptr, nullptr);
    while (child_info) {
        auto child = g_file_enumerator_get_child(e, child_info);
        auto uri = g_file_get_uri(child);
        *queue<<uri;
        g_free(uri);
        g_object_unref(child);
        g_object_unref(child_info);
        child_info = g_file_enumerator_next_file(e, nullptr, nullptr);
    }

    g_file_enumerator_close(e, nullptr, nullptr);
    g_object_unref(e);
}
