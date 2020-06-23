#include "filesyste-mmanager.h"

#include <QApplication>
#include <QMessageBox>
#include <QTimer>


FilesystemManager::FilesystemManager(int& argc, char *argv[], const char *appName)
        : SingleApp(argc, argv, appName, true)
{
    setApplicationVersion("v1.0.0");
    setApplicationName(appName);

    parser.addOption(quitOption);
    parser.addOption(showItemsOption);
    parser.addOption(showFoldersOption);
    parser.addOption(showPropertiesOption);


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
    parser.addOption(quitOption);
    parser.addOption(showItemsOption);
    parser.addOption(showFoldersOption);
    parser.addOption(showPropertiesOption);

    const QStringList args = QString(msg).split(' ');

    parser.process(args);
    if (parser.isSet(quitOption)) {
        QTimer::singleShot(1, [=]() {
            qApp->quit();
        });
        return;
    }

    // FIXME:
}
