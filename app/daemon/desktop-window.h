#ifndef DESKTOPWINDOW_H
#define DESKTOPWINDOW_H

#include <QMainWindow>

class QLabel;
class QSettings;
class QListView;
class QGSettings;
class QStackedLayout;
class DesktopIconView;
class QVariantAnimation;
class QGraphicsOpacityEffect;

/**
 * @brief 桌面屏幕类,主要作用:展示桌面上的图标、桌面背景显示、提供桌面鼠标右键点击菜单
 *
 */
class DesktopWindow : public QMainWindow
{
    Q_OBJECT
public:
    DesktopWindow (QScreen *screen, bool isPrimary, QWidget *parent = nullptr);
    ~DesktopWindow ();

public:
    bool getIsPrimary ();
    QScreen *getScreen ();
    DesktopIconView *getView ();
    const QColor getCurrentColor ();
    void setScreen (QScreen *screen);
    static void gotoSetBackground ();
    const QString getCurrentBgPath ();
    void setIsPrimary (bool isPrimary);

Q_SIGNALS:
    void checkWindow ();
    void changeBg (const QString &bgPath);

public Q_SLOTS:
    void slotUpdateView ();
    void slotConnectSignal ();
    void slotDisconnectSignal ();
    void slotUpdateWinGeometry ();
    void slotScaleBg (const QRect &geometry);
    void slotSetBgPath (const QString &bgPath);
    void slotGeometryChangedProcess (const QRect &geometry);
    void slotVirtualGeometryChangedProcess (const QRect &geometry);
    void slotAvailableGeometryChangedProcess (const QRect &geometry);

protected:
    void paintEvent (QPaintEvent *e) override;

protected Q_SLOTS:
    void slotSetBg (const QColor &color);
    void slotSetBg (const QString &bgPath);

protected:
    void initShortcut ();
    void initGSettings ();

private:
    /**
     * @brief 是否是主屏幕
     */
    bool mIsPrimary;
    QScreen *mScreen;

    /**
     * @brief 屏幕图片
     */
    QPixmap mBgFontPixmap;

    /**
     * @brief 屏幕图片
     */
    QPixmap mBgBackPixmap;

    /**
     * @brief 当前屏幕背景图片路径
     */
    QString mCurrentBgPath;

    /**
     * @brief 当前屏幕的视图(M/V模型中的View)
     */
    DesktopIconView *mView;
    QStackedLayout *mLayout;

    /**
     * @brief 是否使用纯色背景
     */
    bool mUsePureColor = false;
    QPixmap mBgFontCachePixmap;
    QPixmap mBgBackCachePixmap;

    /**
     * @brief 全局设置
     */
    QGSettings *mBgSettings = nullptr;
    QVariantAnimation *mOpacity = nullptr;

    /**
     * @brief 默认设置(备份)
     */
    QSettings *mBackupSetttings = nullptr;
    QGraphicsOpacityEffect *mOpacityEffect;
    QColor mColorToBeSet = Qt::transparent;
    QColor mLastPureColor = Qt::transparent;
};

#endif // DESKTOPWINDOW_H
