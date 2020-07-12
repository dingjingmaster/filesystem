#include "filesystem-manager.h"
#include "fm-window.h"

#include <glib.h>
#include <file-utils.h>
#include <glib/gprintf.h>
#include <window/main-window.h>
#include <syslog/clib_syslog.h>
#include <vfs/search-vfs-register.h>
#include <window/properties-window.h>
#include <view/directory-view-widget.h>
#include <kwindow-system/kwindowsystem.h>
#include <view/directory-view-container.h>
#include <QtWidgets/private/qwidgetresizehandler_p.h>

#include <QFile>
#include <QTimer>
#include <QObject>
#include <QTranslator>
#include <QMessageBox>
#include <QApplication>
#include <QStyleFactory>


FilesystemManager::FilesystemManager(int& argc, char *argv[], const char *appName) : SingleApp(argc, argv, appName, true)
{
    setApplicationVersion("v1.0.0");
    setApplicationName(appName);

//    QFile file(":/desktop-style.qss");
//    file.open(QFile::ReadOnly);
////    qDebug() << file.readAll();
//    setStyleSheet(QString::fromLatin1(file.readAll()));
//    file.close();

    QTranslator *ts = new QTranslator (this);
    ts->load("/usr/share/graceful/filesystem-manager_" + QLocale::system().name());
    QApplication::installTranslator(ts);

    mParser.addOption(mQuitOption);
    mParser.addOption(mShowItemsOption);
    mParser.addOption(mShowFoldersOption);
    mParser.addOption(mShowPropertiesOption);

    mParser.addPositionalArgument("files", tr("open Files or directories"), tr("[FILE1, FILE2, ...]"));

    if (this->isSecondary()) {
        CT_SYSLOG(LOG_DEBUG, "%s", "second file-manager instance");
        mParser.addHelpOption();
        mParser.addVersionOption();
        if (this->arguments().count() == 2 && arguments().last() == ".") {
            QStringList args;
            auto dir = g_get_current_dir();
            args << "graceful-filemanager" << dir;
            g_free(dir);
            auto message = args.join(' ').toUtf8();
            sendMessage(message);
            return;
        }
        mParser.process(arguments());
        auto message = this->arguments().join(' ').toUtf8();
        sendMessage(message);
        return;
    }

    if (this->isPrimary()) {
        connect(this, &SingleApp::receivedMessage, this, &FilesystemManager::slotParseCommandLine);
    }

    auto message = this->arguments().join(' ').toUtf8();
    slotParseCommandLine(this->instanceId(), message);

    auto testIcon = QIcon::fromTheme("folder");
    if (testIcon.isNull()) {
        QIcon::setThemeName("ukui-icon-theme-default");
        if (QStyleFactory::keys().contains("gtk2")) {
            setStyle("gtk2");
        }
        CT_SYSLOG(LOG_WARNING, "cannot get system icon theme");
    }

    SearchVFSRegister::registSearchVFS();
}

void FilesystemManager::about()
{
    QMessageBox::about(nullptr, "graceful filemanager", "Author:\t\t\t\n\n\n");
}

void FilesystemManager::help()
{
    about();
}

void FilesystemManager::slotParseCommandLine (quint32 id, QByteArray msg)
{
    QCommandLineParser parser;
    if (mFirstParse) {
        parser.addHelpOption();
        parser.addVersionOption();
        mFirstParse = false;
    }
    parser.addOption(mQuitOption);
    parser.addOption(mShowItemsOption);
    parser.addOption(mShowFoldersOption);
    parser.addOption(mShowPropertiesOption);

    const QStringList args = QString(msg).split(' ');
    CT_SYSLOG(LOG_DEBUG, "process: %s", QString(msg).toUtf8().data());

    parser.process(args);
    if (parser.isSet(mQuitOption)) {
        CT_SYSLOG(LOG_DEBUG, "graceful-filemanager -q");
        QTimer::singleShot(1, [=]() {
            qApp->quit();
        });
        return;
    }

    if (!parser.optionNames().isEmpty()) {
        if (parser.isSet(mShowItemsOption)) {
            QHash<QString, QStringList> itemHash;
            auto uris = FileUtils::toDisplayUris(parser.positionalArguments());
            for (auto uri : uris) {
                auto parentUri = FileUtils::getParentUri(uri);
                if (itemHash.value(parentUri).isEmpty()) {
                    QStringList l;
                    l<<uri;
                    itemHash.insert(parentUri, l);
                } else {
                    auto l = itemHash.value(parentUri);
                    l<<uri;
                    itemHash.remove(parentUri);
                    itemHash.insert(parentUri, l);
                }
            }
            auto parentUris = itemHash.keys();

            for (auto parentUri : parentUris) {
                auto window = new MainWindow(parentUri);
                connect(window, &MainWindow::locationChangeEnd, [=]() {
                    QTimer::singleShot(500, [=] {
                        window->getCurrentPage()->getView()->setSelections(itemHash.value(parentUri));
                        window->getCurrentPage()->getView()->scrollToSelection(itemHash.value(parentUri).first());
                    });
                });
                window->show();
                KWindowSystem::raiseWindow(window->winId());
            }
        }

        if (parser.isSet(mShowFoldersOption)) {
            QStringList uris = FileUtils::toDisplayUris(parser.positionalArguments());
            auto window = new MainWindow(uris.first());
            uris.removeAt(0);
            if (!uris.isEmpty()) {
                window->slotAddNewTabs(uris);
            }
            window->show();
            KWindowSystem::raiseWindow(window->winId());
        }
        if (parser.isSet(mShowPropertiesOption)) {
            QStringList uris = FileUtils::toDisplayUris(parser.positionalArguments());

            PropertiesWindow *window = new PropertiesWindow(uris);
            window->show();
            KWindowSystem::raiseWindow(window->winId());
        }
    } else {
        CT_SYSLOG(LOG_DEBUG, "There is no param set.");
        if (!parser.positionalArguments().isEmpty()) {
            QStringList uris = FileUtils::toDisplayUris(parser.positionalArguments());
            CT_SYSLOG(LOG_DEBUG, "show in directory: '%s'", uris.first().toUtf8().constData());
            auto window = new MainWindow(uris.first());
            uris.removeAt(0);
            if (!uris.isEmpty()) {
                window->slotAddNewTabs(uris);
            }
            window->setAttribute(Qt::WA_DeleteOnClose);
            window->show();
            KWindowSystem::raiseWindow(window->winId());
        } else {
            auto window = new MainWindow;
            window->setAttribute(Qt::WA_DeleteOnClose);
            CT_SYSLOG(LOG_DEBUG, "show window!");
            window->show();
            KWindowSystem::raiseWindow(window->winId());
        }
    }

    connect(this, &QApplication::paletteChanged, this, [=](const QPalette &pal) {
        for (auto w : allWidgets()) {
            w->setPalette(pal);
            w->update();
        }
    });

    Q_UNUSED(id);
}
