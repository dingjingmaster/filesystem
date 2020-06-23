#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

#include <single-app.h>

#include <QCommandLineOption>
#include <QCommandLineParser>

class FilesystemManager : SingleApp
{
    Q_OBJECT
public:
    /**
     * @brief 主程序入口
     * @param argc 命令行参数个数
     * @param argv 命令参数
     * @param appName app名
     */
    FilesystemManager(int& argc, char* argv[], const char* appName = "graceful-filesystem");

    /**
     * @brief 关于
     */
    static void about ();

    /**
     * @brief 帮助
     * @todo 目前帮助和关于是一样的
     */
    static void help ();

protected Q_SLOTS:
    /**
     * @brief 解析命令行参数
     */
    void slotParseCommandLine (quint32 id, QByteArray msg);

private:
    QCommandLineParser parser;
    QCommandLineOption quitOption = QCommandLineOption(QStringList()<<"q"<<"quit", tr("Close all peony-qt windows and quit"));
    QCommandLineOption showItemsOption = QCommandLineOption(QStringList()<<"i"<<"show-items", tr("Show items"));
    QCommandLineOption showFoldersOption = QCommandLineOption(QStringList()<<"f"<<"show-folders", tr("Show folders"));
    QCommandLineOption showPropertiesOption = QCommandLineOption(QStringList()<<"p"<<"show-properties", tr("Show properties"));

    bool mFirstParse = true;

};

#endif // FILESYSTEMMANAGER_H
