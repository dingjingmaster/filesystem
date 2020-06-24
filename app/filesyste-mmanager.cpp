#include "filesyste-mmanager.h"

#include <app/common/file-utils.h>
#include <glib.h>
#include <glib/gprintf.h>

#include <QApplication>
#include <QMessageBox>
#include <QTimer>


FilesystemManager::FilesystemManager(int& argc, char *argv[], const char *appName)
        : SingleApp(argc, argv, appName, true)
{
    setApplicationVersion("v1.0.0");
    setApplicationName(appName);

    mParser.addOption(mQuitOption);
    mParser.addOption(mShowItemsOption);
    mParser.addOption(mShowFoldersOption);
    mParser.addOption(mShowPropertiesOption);

    if (this->isSecondary()) {
        mParser.addHelpOption();
        mParser.addVersionOption();
        if (this->arguments().count() == 2 && arguments().last() == ".") {
            QStringList args;
            auto dir = g_get_current_dir();
            args<<"filesystem manager"<<dir;
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




}

void FilesystemManager::about()
{
    QMessageBox::about(nullptr, "graceful filesystem", "Author:\n\tding jing <dingjing@live.cn>\n");
}

void FilesystemManager::help()
{
    about();
}

void FilesystemManager::slotParseCommandLine(quint32 id, QByteArray msg)
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

    parser.process(args);
    if (parser.isSet(mQuitOption)) {
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
//                auto window = new MainWindow(parentUri);
                //Peony::FMWindow *window = new Peony::FMWindow(parentUri);
//                connect(window, &MainWindow::locationChangeEnd, [=]() {
//                    QTimer::singleShot(500, [=] {
//                        window->getCurrentPage()->getView()->setSelections(itemHash.value(parentUri));
//                        window->getCurrentPage()->getView()->scrollToSelection(itemHash.value(parentUri).first());
//                    });
//                });
//                window->show();
//                KWindowSystem::raiseWindow(window->winId());
            }
//            if (parser.isSet(showFoldersOption)) {
//                QStringList uris = Peony::FileUtils::toDisplayUris(parser.positionalArguments());
//                auto window = new MainWindow(uris.first());
//                //Peony::FMWindow *window = new Peony::FMWindow(uris.first());
//                uris.removeAt(0);
//                if (!uris.isEmpty()) {
//                    window->addNewTabs(uris);
//                }
//                window->show();
//                KWindowSystem::raiseWindow(window->winId());
//            }
//            if (parser.isSet(showPropertiesOption)) {
//                QStringList uris = Peony::FileUtils::toDisplayUris(parser.positionalArguments());

//                Peony::PropertiesWindow *window = new Peony::PropertiesWindow(uris);
//                window->show();
//                KWindowSystem::raiseWindow(window->winId());
//            }
        } else {
            if (!parser.positionalArguments().isEmpty()) {
                QStringList uris = FileUtils::toDisplayUris(parser.positionalArguments());
                //auto window = new Peony::FMWindow(uris.first());
//                auto window = new MainWindow(uris.first());
//                uris.removeAt(0);
//                if (!uris.isEmpty()) {
//                    window->addNewTabs(uris);
//                }
//                window->setAttribute(Qt::WA_DeleteOnClose);
//                window->show();
//                KWindowSystem::raiseWindow(window->winId());
            } else {
//                auto window = new MainWindow;
//                //auto window = new Peony::FMWindow;
//                window->setAttribute(Qt::WA_DeleteOnClose);
//                window->show();
//                KWindowSystem::raiseWindow(window->winId());
            }
        }
    }
}
