#include "pathbar-model.h"

#include <file-utils.h>
#include <file/file-info.h>
#include <file/file-enumerator.h>

PathBarModel::PathBarModel(QObject *parent) : QStringListModel (parent)
{

}

QString PathBarModel::findDisplayName(const QString &uri)
{
    if (mUriDisplayNameHash.find(uri)->isNull()) {
        return FileUtils::getFileDisplayName(uri);
    } else {
        return mUriDisplayNameHash.value(uri);
    }
}

void PathBarModel::setRootUri(const QString &uri, bool force)
{
    QStringList l;

    if (!force) {
        if (uri.contains("////")) {
            return;
        }

        if (mCurrentUri == uri) {
            return;
        }

        auto file = wrapGFile(g_file_new_for_uri(uri.toUtf8().constData()));
        if (!g_file_query_exists(file.get()->get(), nullptr)) {
            return;
        }
    }

    if (uri.startsWith("search://")) {
        return;
    }


    if (uri.startsWith("trash://")) {
        return;
    }

    beginResetModel();

    mCurrentUri = uri;

    FileEnumerator e;
    e.setEnumerateDirectory(uri);
    e.enumerateSync();
    auto infos = e.getChildren();
    if (infos.isEmpty()) {
        endResetModel();
        Q_EMIT updated();
        return;
    }

    mUriDisplayNameHash.clear();
    for (auto info : infos) {
        if (!(info->isDir() || info->isVolume())) {
            continue;
        }

        //skip the hidden file.
        QString display_name = FileUtils::getFileDisplayName(info->uri());
        if (display_name.startsWith(".")) {
            continue;
        }

        l << info->uri ();
        mUriDisplayNameHash.insert(info->uri(), display_name);
    }
    setStringList(l);
    sort(0);
    endResetModel();

    Q_EMIT updated();
}

void PathBarModel::setRootPath(const QString &path, bool force)
{
    setRootUri("file://" + path, force);
}

QString PathBarModel::currentDirUri()
{
    return mCurrentUri;
}
