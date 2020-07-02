#include "file-launch-action.h"

#include "file-info.h"
#include "file-info-job.h"
#include <gio/gdesktopappinfo.h>

#include <QUrl>
#include <QProcess>
#include <QMessageBox>
#include <QPushButton>


FileLaunchAction::FileLaunchAction(const QString &uri, GAppInfo *app_info, bool forceWithArg, QObject *parent) : QAction(parent)
{
    mUri = uri;
    mAppInfo = static_cast<GAppInfo*>(g_object_ref(app_info));
    mForceWithArg = forceWithArg;

    if (!isValid())
        return;

    GThemedIcon *icon = G_THEMED_ICON(g_app_info_get_icon(mAppInfo));
    const char * const * icon_names = g_themed_icon_get_names(icon);

    if (icon_names)
        mIcon = QIcon::fromTheme(*icon_names);
    setIcon(mIcon);
    mInfoName = g_app_info_get_name(mAppInfo);
    setText(mInfoName);
    mInfoDisplayName = g_app_info_get_display_name(mAppInfo);

    connect(this, &QAction::triggered, [=]() {
        this->lauchFileAsync(mForceWithArg, true);
    });
}

FileLaunchAction::~FileLaunchAction()
{
    if (mAppInfo) {
        g_object_unref(mAppInfo);
    }
}

GAppInfo *FileLaunchAction::gAppInfo()
{
    return mAppInfo;
}

const QString FileLaunchAction::getUri()
{
    return mUri;
}

bool FileLaunchAction::isDesktopFileAction()
{
    auto info = FileInfo::fromUri(mUri, false);
    if (info->isEmptyInfo()) {
        FileInfoJob j(info);
        j.querySync();
    }
    return info->isDesktopFile();
}

const QString FileLaunchAction::getAppInfoName()
{
    return mInfoName;
}

const QString FileLaunchAction::getAppInfoDisplayName()
{
    return mInfoDisplayName;
}

bool FileLaunchAction::isValid()
{
    return G_IS_APP_INFO(mAppInfo);
}

void FileLaunchAction::execFile()
{
    QUrl url = mUri;
    char *quote = g_shell_quote(url.path().toUtf8());
    GAppInfo *app_info = g_app_info_create_from_commandline(quote, nullptr, G_APP_INFO_CREATE_NONE, nullptr);
    g_app_info_launch(app_info, nullptr, nullptr, nullptr);
    g_object_unref(app_info);
    g_free(quote);
}

void FileLaunchAction::execFileInterm()
{
    QUrl url = mUri;
    char *quote = g_shell_quote(url.path().toUtf8());
    GAppInfo *app_info = g_app_info_create_from_commandline(quote, nullptr, G_APP_INFO_CREATE_NEEDS_TERMINAL, nullptr);
    g_app_info_launch(app_info, nullptr, nullptr, nullptr);
    g_object_unref(app_info);
    g_free(quote);
}

void FileLaunchAction::lauchFileSync(bool forceWithArg, bool skipDialog)
{
    auto fileInfo = FileInfo::fromUri(mUri, false);
    if (fileInfo->isEmptyInfo()) {
        FileInfoJob j(fileInfo);
        j.querySync();
    }

    bool executable = fileInfo->canExecute();
    bool isAppImage = fileInfo->type() == "application/vnd.appimage";
    if (isAppImage) {
        if (executable) {
            QUrl url = mUri;
            auto path = url.path();

            QProcess p;
            p.setProgram(path);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            p.startDetached();
#else
            p.startDetached(path);
#endif
            return;
        }
    }

    if (executable && !isDesktopFileAction() && !skipDialog) {
        QMessageBox msg;
        auto exec = msg.addButton(tr("Execute Directly"), QMessageBox::ButtonRole::ActionRole);
        auto execTerm = msg.addButton(tr("Execute in Terminal"), QMessageBox::ButtonRole::ActionRole);
        auto defaultAction = msg.addButton("By Default App", QMessageBox::ButtonRole::ActionRole);
        msg.addButton(QMessageBox::Cancel);

        msg.setText(tr("Detected launching an executable file %1, you want?").arg(fileInfo->displayName()));
        msg.exec();
        auto button = msg.clickedButton();
        if (button == exec) {
            execFile();
            return;
        } else if (button == execTerm) {
            execFileInterm();
            return;
        } else if (button == defaultAction) {
            //skip
        } else {
            return;
        }
    }

    if (!isValid()) {
        QMessageBox::critical(nullptr, tr("Open Failed"), tr("Can not open %1, file not exist, is it deleted?").arg(mUri));
        return;
    }

    if (isDesktopFileAction() && !forceWithArg) {
        g_app_info_launch(mAppInfo, nullptr, nullptr, nullptr);
    } else {
        GList *l = nullptr;
        char *uri = g_strdup(mUri.toUtf8().constData());
        l = g_list_prepend(l, uri);
        g_app_info_launch_uris(mAppInfo, l, nullptr, nullptr);
        g_list_free_full(l, g_free);
    }

    return;
    if (isDesktopFileAction() && !forceWithArg) {
        auto desktopInfo = G_DESKTOP_APP_INFO(mAppInfo);
        g_desktop_app_info_launch_uris_as_manager (desktopInfo, nullptr, nullptr, G_SPAWN_DEFAULT, nullptr, nullptr, nullptr, nullptr, nullptr);
    } else {
        g_app_info_launch_default_for_uri(mUri.toUtf8().constData(), nullptr, nullptr);
    }
}

void FileLaunchAction::lauchFileAsync(bool forceWithArg, bool skipDialog)
{
    auto fileInfo = FileInfo::fromUri(mUri, false);
    if (fileInfo->isEmptyInfo()) {
        FileInfoJob j(fileInfo);
        j.querySync();
    }

    bool executable = fileInfo->canExecute();
    bool isAppImage = fileInfo->type() == "application/vnd.appimage";
    if (isAppImage) {
        if (executable) {
            QUrl url = mUri;
            auto path = url.path();

            QProcess p;
            p.setProgram(path);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            p.startDetached();
#else
            p.startDetached(path);
#endif
            return;
        }
    }

    if (executable && !isDesktopFileAction() && !skipDialog) {
        QMessageBox msg;
        auto exec = msg.addButton(tr("Execute Directly"), QMessageBox::ButtonRole::ActionRole);
        auto execTerm = msg.addButton(tr("Execute in Terminal"), QMessageBox::ButtonRole::ActionRole);
        auto defaultAction = msg.addButton(tr("By Default App"), QMessageBox::ButtonRole::ActionRole);
        msg.addButton(QMessageBox::Cancel);

        msg.setWindowTitle(tr("Launch Options"));
        msg.setText(tr("Detected launching an executable file %1, you want?").arg(fileInfo->displayName()));
        msg.exec();
        auto button = msg.clickedButton();
        if (button == exec) {
            execFile();
            return;
        } else if (button == execTerm) {
            execFileInterm();
            return;
        } else if (button == defaultAction) {
            //skip
        } else {
            return;
        }
    }

    if (!isValid()) {
        bool isReadable = fileInfo->canRead();
        if (!isReadable) {
            if (fileInfo->isSymbolLink())
                QMessageBox::critical(nullptr, tr("Open Link failed"), tr("File not exist, is it deleted or moved to other path?"));
            else
                QMessageBox::critical(nullptr, tr("Open Failed"), tr("Can not open %1, Please confirm you have the right authority.").arg(mUri));
        } else {
            auto result = QMessageBox::question(nullptr, tr("Error"), tr("Can not get a default application for openning %1, do you want open it with text format?").arg(mUri));
            if (result == QMessageBox::Yes) {
                GAppInfo *text_info = g_app_info_get_default_for_type("text/plain", false);
                GList *l = nullptr;
                char *uri = g_strdup(mUri.toUtf8().constData());
                l = g_list_prepend(l, uri);
#if GLIB_CHECK_VERSION(2, 60, 0)
                g_app_info_launch_uris_async(text_info, l, nullptr, nullptr, nullptr, nullptr);
#else
                g_app_info_launch_uris(text_info, l, nullptr, nullptr);
#endif
                g_list_free_full(l, g_free);
                g_object_unref(text_info);
            }
        }
        return;
    }

    if (isDesktopFileAction() && !forceWithArg) {
#if GLIB_CHECK_VERSION(2, 60, 0)
        g_app_info_launch_uris_async(mAppInfo, nullptr, nullptr, nullptr, nullptr, nullptr);
#else
        g_app_info_launch_uris(mAppInfo, nullptr, nullptr, nullptr);
#endif
    } else {
        GList *l = nullptr;
        char *uri = g_strdup(mUri.toUtf8().constData());
        l = g_list_prepend(l, uri);
#if GLIB_CHECK_VERSION(2, 60, 0)
        g_app_info_launch_uris_async(mAppInfo, l, nullptr, nullptr, nullptr, nullptr);
#else
        g_app_info_launch_uris(mAppInfo, l, nullptr, nullptr);
#endif
        g_list_free_full(l, g_free);
    }

    return;
    if (isDesktopFileAction() && !forceWithArg) {
        auto desktopInfo = G_DESKTOP_APP_INFO(mAppInfo);
        g_desktop_app_info_launch_uris_as_manager (desktopInfo, nullptr, nullptr, G_SPAWN_DEFAULT, nullptr, nullptr, nullptr, nullptr, nullptr);
    } else {
#if GLIB_CHECK_VERSION(2, 50, 0)
        g_app_info_launch_default_for_uri_async(mUri.toUtf8().constData(), nullptr, nullptr, nullptr, nullptr);
#else
        g_app_info_launch_default_for_uri(mUri.toUtf8().constData(), nullptr, nullptr);
#endif
    }
}
