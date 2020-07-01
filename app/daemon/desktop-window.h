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
    bool mIsPrimary;
    QScreen *mScreen;
    QPixmap mBgFontPixmap;
    QPixmap mBgBackPixmap;
    QString mCurrentBgPath;
    DesktopIconView *mView;
    QStackedLayout *mLayout;
    bool mUsePureColor = false;
    QPixmap mBgFontCachePixmap;
    QPixmap mBgBackCachePixmap;
    QGSettings *mBgSettings = nullptr;
    QVariantAnimation *mOpacity = nullptr;
    QSettings *mBackupSetttings = nullptr;
    QGraphicsOpacityEffect *mOpacityEffect;
    QColor mColorToBeSet = Qt::transparent;
    QColor mLastPureColor = Qt::transparent;
};

#endif // DESKTOPWINDOW_H
