#include "filesystem-manager.h"
#include "fm-window.h"

#include <glib.h>
#include <file-utils.h>
#include <clib_syslog.h>
#include <glib/gprintf.h>
#include <window/main-window.h>
#include <window/properties-window.h>
#include <vfs/search-vfs-register.h>
#include <kwindow-system/kwindowsystem.h>
#include <view/directory-view-container.h>
#include <view/directory-view-widget.h>

#include <QTimer>
#include <QObject>
#include <QTranslator>
#include <QMessageBox>
#include <QApplication>


FilesystemManager::FilesystemManager(int& argc, char *argv[], const char *appName) : SingleApp(argc, argv, appName, true)
{
    setApplicationVersion("v1.0.0");
    setApplicationName(appName);

    QTranslator *ts = new QTranslator (this);
    ts->load("/usr/share/filesystem-manager/filesystem-manager_" + QLocale::system().name());
    QApplication::installTranslator(ts);

    mParser.addOption(mQuitOption);
    mParser.addOption(mShowItemsOption);
    mParser.addOption(mShowFoldersOption);
    mParser.addOption(mShowPropertiesOption);

    mParser.addPositionalArgument("files", tr("open Files or directories"), tr("[FILE1, FILE2, ...]"));

    if (this->isSecondary()) {
        CT_SYSLOG(LOG_DEBUG, "%s", "第二个filesystem-manager实例");
        mParser.addHelpOption();
        mParser.addVersionOption();
        if (this->arguments().count() == 2 && arguments().last() == ".") {
            QStringList args;
            auto dir = g_get_current_dir();
            args << "filesystem manager" << dir;
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

    SearchVFSRegister::registSearchVFS();
}

void FilesystemManager::about()
{
    QMessageBox::about(nullptr, "graceful filesystem", "Author:\n\tding jing <dingjing@live.cn>\n");
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
    CT_SYSLOG(LOG_DEBUG, "process: %s", QString(msg).toUtf8().data())
    parser.process(args);
    if (parser.isSet(mQuitOption)) {
        CT_SYSLOG(LOG_DEBUG, "退出");
        QTimer::singleShot(1, [=]() {
            qApp->quit();
        });
        return;
    }

    if (!mParser.optionNames().isEmpty()) {
        if (mParser.isSet(mShowItemsOption)) {
            QHash<QString, QStringList> itemHash;
            auto uris = FileUtils::toDisplayUris(parser.positionalArguments());
            for (auto uri : uris) {
                auto parentUri = FileUtils::getParentUri(uri);
                if (itemHash.value(parentUri).isEmpty()) {
                    QStringList l;
                    l << uri;
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
            if (parser.isSet(mShowFoldersOption)) {
                QStringList uris = FileUtils::toDisplayUris(parser.positionalArguments());
                auto window = new MainWindow(uris.first());
                uris.removeAt(0);
                if (!uris.isEmpty()) {
                    CT_SYSLOG(LOG_DEBUG, "添加新的widget到tabwidget")
                    window->slotAddNewTabs(uris);
                }
                window->show();
                KWindowSystem::raiseWindow(window->winId());
            }
            if (parser.isSet(mShowPropertiesOption)) {
                QStringList uris = FileUtils::toDisplayUris(parser.positionalArguments());
                CT_SYSLOG(LOG_DEBUG, "展示属性窗口");
                PropertiesWindow *window = new PropertiesWindow(uris);
                window->show();
                KWindowSystem::raiseWindow(window->winId());
            }
        } else {
            if (!parser.positionalArguments().isEmpty()) {
                QStringList uris = FileUtils::toDisplayUris(parser.positionalArguments());
                auto window = new FMWindow(uris.first());
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
                window->show();
                KWindowSystem::raiseWindow(window->winId());
            }
        }
    }
}
