#include "file-info-job.h"
#include "file-info.h"
#include "file-launch-action.h"
#include "file-launch-manager.h"

#include <gio/gdesktopappinfo.h>

#include <QUrl>
#include <file-utils.h>


FileLaunchAction *FileLaunchManager::getDefaultAction(const QString &uri)
{
    auto info = FileInfo::fromUri(uri);
    QString mimeType = info->mimeType();

    if (mimeType.isEmpty()) {
        FileInfoJob job(info);
        job.querySync();
        mimeType = info->mimeType();
    }

    if (info->canExecute() && info->uri().endsWith(".desktop")) {
        QUrl url = uri;
        auto path = url.path();
        GDesktopAppInfo *info = g_desktop_app_info_new_from_filename(path.toUtf8().constData());
        FileLaunchAction *action = new FileLaunchAction(uri, G_APP_INFO(info));
        g_object_unref(info);
        return action;
    } else {
        GAppInfo *info = g_app_info_get_default_for_type(mimeType.toUtf8().constData(), false);
        FileLaunchAction *action = new FileLaunchAction(uri, info);
        g_object_unref(info);
        return action;
    }
}

const QList<FileLaunchAction *> FileLaunchManager::getAllActions(const QString &uri)
{
    GList *app_infos = g_app_info_get_all();
    GList *l = app_infos;
    QList<FileLaunchAction *> actions;
    while (l) {
        auto app_info = static_cast<GAppInfo*>(l->data);
        actions<<new FileLaunchAction(uri, app_info, true);
        g_object_unref(app_info);
        l = l->next;
    }
    return actions;
}

const QList<FileLaunchAction *> FileLaunchManager::getFallbackActions(const QString &uri)
{
    auto info = FileInfo::fromUri(uri);
    QString mimeType = info->mimeType();
    if (mimeType.isEmpty()) {
        FileInfoJob job(info);
        job.querySync();
        mimeType = info->mimeType();
    }
    GList *app_infos = g_app_info_get_fallback_for_type(mimeType.toUtf8().constData());
    GList *l = app_infos;
    QList<FileLaunchAction *> actions;
    while (l) {
        auto app_info = static_cast<GAppInfo*>(l->data);
        actions<<new FileLaunchAction(uri, app_info, true);
        g_object_unref(app_info);
        l = l->next;
    }
    return actions;
}

const QList<FileLaunchAction *> FileLaunchManager::getRecommendActions(const QString &uri)
{
    auto info = FileInfo::fromUri(uri);
    QString mimeType = info->mimeType();
    if (mimeType.isEmpty()) {
        FileInfoJob job(info);
        job.querySync();
        mimeType = info->mimeType();
    }
    GList *app_infos = g_app_info_get_recommended_for_type(mimeType.toUtf8().constData());
    GList *l = app_infos;
    QList<FileLaunchAction *> actions;
    while (l) {
        auto app_info = static_cast<GAppInfo*>(l->data);
        actions<<new FileLaunchAction(uri, app_info, true);
        g_object_unref(app_info);
        l = l->next;
    }
    return actions;
}

const QList<FileLaunchAction *> FileLaunchManager::getAllActionsForType(const QString &uri)
{
    auto info = FileInfo::fromUri(uri);
    QString mimeType = info->mimeType();
    if (mimeType.isEmpty()) {
        FileInfoJob job(info);
        job.querySync();
        mimeType = info->mimeType();
    }
    GList *app_infos = g_app_info_get_all_for_type(mimeType.toUtf8().constData());
    GList *l = app_infos;
    QList<FileLaunchAction *> actions;
    while (l) {
        auto app_info = static_cast<GAppInfo*>(l->data);
        actions<<new FileLaunchAction(uri, app_info, true);
        g_object_unref(app_info);
        l = l->next;
    }
    return actions;
}

void FileLaunchManager::setDefaultLauchAction(const QString &uri, FileLaunchAction *action)
{
    auto info = FileInfo::fromUri(uri, false);
    if (info->mimeType().isEmpty()) {
        FileInfoJob job(info);
        job.querySync();
    }
    g_app_info_set_as_default_for_type(action->gAppInfo(), info->mimeType().toUtf8(), nullptr);
}

void FileLaunchManager::openSync(const QString &uri, bool forceWithArg, bool skipDialog)
{
    QString tmp = uri;
    auto targetUri = FileUtils::getTargetUri(uri);
    if (targetUri.isNull()) {
        tmp = targetUri;
    }
    auto action = getDefaultAction(tmp);
    action->lauchFileSync(forceWithArg, skipDialog);
    action->deleteLater();
}

void FileLaunchManager::openAsync(const QString &uri, bool forceWithArg, bool skipDialog)
{
    QString tmp = uri;
    auto targetUri = FileUtils::getTargetUri(uri);
    if (!targetUri.isNull()) {
        tmp = targetUri;
    }
    auto action = getDefaultAction(tmp);
    action->lauchFileAsync(forceWithArg, skipDialog);
    action->deleteLater();
}

FileLaunchManager::FileLaunchManager(QObject *parent) : QObject(parent)
{

}
