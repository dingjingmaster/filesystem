#ifndef FMDESKTOPAPPLICATION_H
#define FMDESKTOPAPPLICATION_H

#include "desktop-icon-view.h"
#include "desktop-window.h"

#include <single-app/single-app.h>

#include <QScreen>

/**
 * @brief graceful-desktop 主类,保证系统上此进程唯一,管理所有显示器,解析命令行参数
 * @see SingleApp
 */
class FMDesktopApplication : public SingleApp
{
    Q_OBJECT
public:

    /**
     * @param argc 命令行参数个数
     * @param argv 命令行参数
     * @param applicationName 进程名, SingleApp 需要
     * @see SingleApp
     */
    explicit FMDesktopApplication(int &argc, char *argv[], const char *applicationName = "graceful-desktop");

    /**
     * @brief 获取桌面视图类
     * @see DesktopIconView
     */
    static DesktopIconView *getIconView ();

protected Q_SLOTS:
    /**
     * @brief 查看是否是主屏
     * @param screen 表示可获取到的所有屏幕
     * @see QApplication::primaryScreen()
     */
    bool isPrimaryScreen (QScreen *screen);

    /**
     * @brief 解析命令行参数
     * @param id 共享内存实例 ID
     * @param msg 命令行参数, 以 ' ' 分割
     * @param isPrimary 是否第一个进程实例
     */
    void parseCmd (quint32 id, QByteArray msg, bool isPrimary);

public Q_SLOTS:
    void checkWindowProcess ();

    /**
     * @brief 新的屏幕连接
     */
    void screenAddedProcess (QScreen *screen);

    /**
     * @brief 屏幕移除
     */
    void screenRemovedProcess (QScreen *screen);

    /**
     * @brief 修改背景图片
     */
    void changeBgProcess (const QString& bgPath);

    /**
     * @brief 修改了主屏
     */
    void primaryScreenChangedProcess (QScreen *screen);

    /**
     * @brief 添加屏幕
     */
    void addWindow (QScreen *screen, bool checkPrimay = true);
    void layoutDirectionChangedProcess (Qt::LayoutDirection direction);

private:
    bool mFirstParse = true;

    /**
     * @brief 所有连接的屏幕
     */
    QList<DesktopWindow*> mWindowList;
};

#endif // FMDESKTOPAPPLICATION_H
