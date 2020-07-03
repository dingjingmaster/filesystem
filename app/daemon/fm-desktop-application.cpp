#include "desktop-menu-plugin-manager.h"
#include "fm-dbus-service.h"
#include "fm-desktop-application.h"
#include <syslog/clib_syslog.h>

#include <QFile>
#include <QProcess>
#include <gio/gio.h>
#include <QtConcurrent>
#include <QApplication>
#include <QCommandLineParser>

static void trySetDefaultFolderUrlHandler();

static bool gHasDaemon = false;
static bool gHasDesktop = false;
static DesktopIconView *gDesktopIconView = nullptr;

FMDesktopApplication::FMDesktopApplication(int &argc, char *argv[], const char *applicationName) : SingleApp(argc, argv, applicationName)
{
    setApplicationVersion("v1.0.0");
    setApplicationName(applicationName);

    if (this->isPrimary()) {
        connect(this, &SingleApp::receivedMessage, [=](quint32 id, QByteArray msg) {
            this->parseCmd(id, msg, true);
        });
//        QFile file(":/desktop-icon-view.qss");
//        file.open(QFile::ReadOnly);
//        setStyleSheet(QString::fromLatin1(file.readAll()));
//        file.close();
        DesktopMenuPluginManager::getInstance();
    }

    connect(this, &SingleApp::screenAdded, this, &FMDesktopApplication::screenAddedProcess);
    connect(this, &SingleApp::screenRemoved, this, &FMDesktopApplication::screenRemovedProcess);
    connect(this, &SingleApp::primaryScreenChanged, this, &FMDesktopApplication::primaryScreenChangedProcess);
    connect(this, &SingleApp::layoutDirectionChanged, this, &FMDesktopApplication::layoutDirectionChangedProcess);

    auto message = this->arguments().join(' ').toUtf8();
    parseCmd(this->instanceId(), message, isPrimary());
}

DesktopIconView *FMDesktopApplication::getIconView()
{
    if (!gDesktopIconView) {
        gDesktopIconView = new DesktopIconView;
    }

    return gDesktopIconView;
}

bool FMDesktopApplication::isPrimaryScreen(QScreen *screen)
{
    if (screen == this->primaryScreen()) {
        return true;
    }

    return false;
}

void FMDesktopApplication::parseCmd(quint32 id, QByteArray msg, bool isPrimary)
{
    QCommandLineParser parser;

    QCommandLineOption quitOption(QStringList() << "q" << "quit", tr("Close the peony desktop window"));
    parser.addOption(quitOption);

    QCommandLineOption daemonOption(QStringList() << "d" << "daemon", tr("Take over the dbus service."));
    parser.addOption(daemonOption);

    QCommandLineOption desktopOption(QStringList() << "w" << "desktop-window", tr("Take over the desktop displaying"));
    parser.addOption(desktopOption);

    if (isPrimary) {
        if (mFirstParse) {
            auto helpOption = parser.addHelpOption();
            auto versionOption = parser.addVersionOption();
            mFirstParse = false;
        }

        Q_UNUSED(id)
        const QStringList args = QString(msg).split(' ');

        parser.process(args);
        if (parser.isSet(quitOption)) {
            QTimer::singleShot(1, [=]() {
                qApp->quit();
            });
            return;
        }

        if (parser.isSet(daemonOption)) {
            if (!gHasDaemon) {
                CT_SYSLOG(LOG_DEBUG, "command line -d");
                trySetDefaultFolderUrlHandler();
                FMDBusService *service = new FMDBusService(this);
                connect(service, &FMDBusService::showItemsRequest, [=](const QStringList &urisList) {
                    QProcess p;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
                    p.setProgram("filesystem");
                    p.setArguments(QStringList()<<"--show-items"<<urisList);
                    p.startDetached();
#else
                    p.startDetached("peony", QStringList()<<"--show-items"<<urisList, nullptr);
#endif
                });
                connect(service, &FMDBusService::showFolderRequest, [=](const QStringList &urisList) {
                    QProcess p;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
                    p.setProgram("filesystem");
                    p.setArguments(QStringList()<<"--show-folders"<<urisList);
                    p.startDetached();
#else
                    p.startDetached("peony", QStringList()<<"--show-folders"<<urisList, nullptr);
#endif
                });
                connect(service, &FMDBusService::showItemPropertiesRequest, [=](const QStringList &urisList) {
                    QProcess p;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
                    p.setProgram("filesystem");
                    p.setArguments(QStringList()<<"--show-properties"<<urisList);
                    p.startDetached();
#else
                    p.startDetached("peony", QStringList()<<"--show-properties"<<urisList, nullptr);
#endif
                });
            }
            gHasDaemon = true;
        }

        if (parser.isSet(desktopOption)) {
            CT_SYSLOG(LOG_DEBUG, "command line -w");
            if (!gHasDesktop) {
                getIconView();
                for(auto screen : this->screens()) {
                    CT_SYSLOG(LOG_DEBUG, "add screen:%s", screen->name().toUtf8().data());
                    addWindow(screen);
                }
            }
            gHasDesktop = true;
        }

        connect(this, &QApplication::paletteChanged, this, [=](const QPalette &pal) {
            for (auto w : allWidgets()) {
                w->setPalette(pal);
                w->update();
            }
        });
    } else {
        auto helpOption = parser.addHelpOption();
        auto versionOption = parser.addVersionOption();

        if (arguments().count() < 2) {
            parser.showHelp();
        }
        parser.process(arguments());

        sendMessage(msg);
    }
}

void FMDesktopApplication::checkWindowProcess()
{

}

void FMDesktopApplication::screenAddedProcess(QScreen *screen)
{
    addWindow(screen, false);
}

void FMDesktopApplication::screenRemovedProcess(QScreen *screen)
{
    for(auto win : mWindowList) {
        if (win->getScreen() == screen) {
            mWindowList.removeOne(win);
            win->deleteLater();
        }
    }
}

void FMDesktopApplication::changeBgProcess(const QString &bgPath)
{

}

void FMDesktopApplication::primaryScreenChangedProcess(QScreen *screen)
{
    bool needExchange = false;
    QScreen *preMainScreen = nullptr;
    DesktopWindow *rawPrimaryWindow = nullptr;
    DesktopWindow *currentPrimayWindow = nullptr;

    for(auto win : mWindowList) {
        if (win->centralWidget()) {
            rawPrimaryWindow = win;
        }

        if (win->getScreen() == screen) {
            currentPrimayWindow = win;
        }
    }

    if (rawPrimaryWindow && currentPrimayWindow) {
        currentPrimayWindow->setCentralWidget(getIconView());
        currentPrimayWindow->slotUpdateView();
    }

    return;

    //do not check window need exchange
    //cause we always put the desktop icon view into window
    //which in current primary screen.
    if (needExchange) {
        for(auto win : mWindowList) {
            win->slotDisconnectSignal();
            if (win->getScreen() == preMainScreen) {
                win->setScreen(screen);
                win->setIsPrimary(true);
            }
            else if (win->getScreen() == screen) {
                win->setScreen(preMainScreen);
                win->setIsPrimary(false);
            }
            win->slotConnectSignal();
            win->slotUpdateWinGeometry();
        }
    }
}

void FMDesktopApplication::addWindow(QScreen *screen, bool checkPrimay)
{
    DesktopWindow *window;
    if (checkPrimay) {
        bool isPrimary = isPrimaryScreen(screen);
        window = new DesktopWindow(screen, isPrimary);
        if (isPrimary) {
            window->setCentralWidget(gDesktopIconView);
            window->slotUpdateView();
        }
    } else {
        window = new DesktopWindow(screen, false);
    }

    connect(window, &DesktopWindow::checkWindow, this, &FMDesktopApplication::checkWindowProcess);
    mWindowList << window;

    for (auto window : mWindowList) {
        window->slotUpdateWinGeometry();
    }
}

void FMDesktopApplication::layoutDirectionChangedProcess(Qt::LayoutDirection direction)
{

}


static void trySetDefaultFolderUrlHandler()
{
    QTimer::singleShot(1000, []() {
        QtConcurrent::run([=]() {
            GList *apps = g_app_info_get_all_for_type("inode/directory");
            bool hasFMQtAppInfo = false;
            GList *l = apps;
            while (l) {
                GAppInfo *info = static_cast<GAppInfo*>(l->data);
                QString cmd = g_app_info_get_executable(info);
                if (cmd.contains("fm")) {
                    hasFMQtAppInfo = true;
                    g_app_info_set_as_default_for_type(info, "inode/directory", nullptr);
                    break;
                }
                l = l->next;
            }
            if (apps) {
                g_list_free_full(apps, g_object_unref);
            }

            if (!hasFMQtAppInfo) {
                GAppInfo *peony_qt = g_app_info_create_from_commandline("peony", nullptr, G_APP_INFO_CREATE_SUPPORTS_URIS, nullptr);
                g_app_info_set_as_default_for_type(peony_qt, "inode/directory", nullptr);
                g_object_unref(peony_qt);
            }
            return;
        });
    });
}
