#include "fm-desktop-application.h"

#include "desktop-menu.h"
#include "desktop-window.h"
#include "desktop-icon-view.h"
#include "syslog/clib_syslog.h"

#include <QFile>
#include <QTimer>
#include <QAction>
#include <QScreen>
#include <QPainter>
#include <QProcess>
#include <QX11Info>
#include <QSettings>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <QApplication>
#include <QVariantAnimation>
#include <qgsettings/QGSettings>

#define PICTRUE                 "picture-filename"
#define FALLBACK_COLOR          "primary-color"
#define BACKGROUND_SETTINGS     "org.graceful.background"

DesktopWindow::DesktopWindow(QScreen *screen, bool isPrimary, QWidget *parent) : QMainWindow(parent)
{
    initGSettings();

    CT_SYSLOG(LOG_DEBUG, "DesktopWindow construct ...");
    setWindowTitle(tr("Desktop"));
    mOpacity = new QVariantAnimation(this);
    mOpacity->setDuration (1000);
    mOpacity->setStartValue (double(0));
    mOpacity->setEndValue (double(0.3));
    connect(mOpacity, &QVariantAnimation::valueChanged, this, [=]() {
        this->update();
    });

    connect(mOpacity, &QVariantAnimation::finished, this, [=]() {
        mBgBackPixmap = mBgFontPixmap;
        mBgBackCachePixmap = mBgFontCachePixmap;
        mLastPureColor = mColorToBeSet;
    });

    mScreen = screen;

    //connect(mScreen, &QScreen::availableGeometryChanged, this, &DesktopWindow::updateView);
    slotConnectSignal();

    mIsPrimary = isPrimary;
    setContentsMargins(0, 0, 0, 0);

    CT_SYSLOG(LOG_DEBUG, "is first screen: %s, name:%s", mIsPrimary ? "true" : "false", screen->name().toUtf8().data());

    auto flags = windowFlags() &~Qt::WindowMinMaxButtonsHint;
    setWindowFlags(flags |Qt::FramelessWindowHint);
    setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
    setAttribute(Qt::WA_TranslucentBackground);

#if (QT_VERSION < QT_VERSION_CHECK(5, 7, 0))
    if (QX11Info::isPlatformX11()) {
        Atom mWindowType = XInternAtom(QX11Info::display(), "_NET_WM_WINDOW_TYPE", true);
        Atom mDesktopType = XInternAtom(QX11Info::display(), "_NET_WM_WINDOW_TYPE_DESKTOP", true);
        XDeleteProperty(QX11Info::display(), winId(), mWindowType);
        XChangeProperty(QX11Info::display(), winId(), mWindowType, XA_ATOM, 32, 1, (unsigned char *)&mDesktopType, 1);
    }
#endif

    setGeometry(screen->geometry());
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QMainWindow::customContextMenuRequested, [=](const QPoint &pos) {
        auto contentMargins = contentsMargins();
        auto fixedPos = pos - QPoint(contentMargins.left(), contentMargins.top());
        auto index = FMDesktopApplication::getIconView()->indexAt(fixedPos);
        if (!index.isValid() || !centralWidget()) {
            FMDesktopApplication::getIconView()->clearSelection();
        } else {
            if (!FMDesktopApplication::getIconView()->selectionModel()->selection().indexes().contains(index)) {
                FMDesktopApplication::getIconView()->clearSelection();
                FMDesktopApplication::getIconView()->selectionModel()->select(index, QItemSelectionModel::Select);
            }
        }

        QTimer::singleShot(1, [=]() {
            DesktopMenu menu(FMDesktopApplication::getIconView());
            if (FMDesktopApplication::getIconView()->getSelections().isEmpty()) {
                auto action = menu.addAction(tr("set background"));
                connect(action, &QAction::triggered, [=]() {
                    gotoSetBackground();
                });
            }
            menu.exec(mapToGlobal(pos));
            auto urisToEdit = menu.urisToEdit();
            if (urisToEdit.count() == 1) {
                QTimer::singleShot( 100, this, [=]() {
                    FMDesktopApplication::getIconView()->editUri(urisToEdit.first());
                });
            }
        });
    });

    connect(mScreen, &QScreen::geometryChanged, this, &DesktopWindow::slotGeometryChangedProcess);
    connect(mScreen, &QScreen::virtualGeometryChanged, this, &DesktopWindow::slotVirtualGeometryChangedProcess);

    slotSetBg(getCurrentBgPath());

    CT_SYSLOG(LOG_DEBUG, "DesktopWindow construct ok!");
}

DesktopWindow::~DesktopWindow()
{

}

const QString DesktopWindow::getCurrentBgPath()
{
    CT_SYSLOG(LOG_DEBUG, "beginning ...");
    if (mCurrentBgPath.isEmpty()) {
        if (mBgSettings) {
            mCurrentBgPath = mBgSettings->get("pictureFilename").toString();
        } else {
            mCurrentBgPath = mBackupSetttings->value("pictrue").toString();
        }
    }
    CT_SYSLOG(LOG_DEBUG, "end!");

    // FIXME://
    return "/usr/share/backgrounds/09.jpg";
    return mCurrentBgPath;
}

void DesktopWindow::setIsPrimary(bool isPrimary)
{
    mIsPrimary = isPrimary;
}

void DesktopWindow::slotUpdateView()
{
    CT_SYSLOG(LOG_DEBUG, "beginning ...");
    auto avaliableGeometry = mScreen->availableGeometry();
    auto geomerty = mScreen->geometry();
    int top = qAbs(avaliableGeometry.top() - geomerty.top());
    int left = qAbs(avaliableGeometry.left() - geomerty.left());
    int bottom = qAbs(avaliableGeometry.bottom() - geomerty.bottom());
    int right = qAbs(avaliableGeometry.right() - geomerty.right());
    if (top > 200 | left > 200 | bottom > 200 | right > 200) {
        setContentsMargins(0, 0, 0, 0);
    } else {
        setContentsMargins(left, top, right, bottom);
    }
    CT_SYSLOG(LOG_DEBUG, "end!");
}

void DesktopWindow::slotConnectSignal()
{
    CT_SYSLOG(LOG_DEBUG, "beginning ...");
    connect(mScreen, &QScreen::geometryChanged, this, &DesktopWindow::slotGeometryChangedProcess);
    connect(mScreen, &QScreen::virtualGeometryChanged, this, &DesktopWindow::slotVirtualGeometryChangedProcess);
    connect(mScreen, &QScreen::availableGeometryChanged, this, &DesktopWindow::slotAvailableGeometryChangedProcess);
    CT_SYSLOG(LOG_DEBUG, "end!");
}

void DesktopWindow::slotDisconnectSignal()
{
    CT_SYSLOG(LOG_DEBUG, "beginning ...");
    disconnect(mScreen, &QScreen::geometryChanged, this, &DesktopWindow::slotGeometryChangedProcess);
    disconnect(mScreen, &QScreen::virtualGeometryChanged, this, &DesktopWindow::slotVirtualGeometryChangedProcess);
    if (mIsPrimary) {
        disconnect(mScreen, &QScreen::availableGeometryChanged, this, &DesktopWindow::slotAvailableGeometryChangedProcess);
    }
    CT_SYSLOG(LOG_DEBUG, "end!");
}

void DesktopWindow::slotUpdateWinGeometry()
{
    CT_SYSLOG(LOG_DEBUG, "beginning ...");
    auto screenName = mScreen->name();
    auto screenSize = mScreen->size();
    auto g = getScreen()->geometry();
    auto vg = getScreen()->virtualGeometry();
    auto ag = getScreen()->availableGeometry();

    slotScaleBg(g);

    auto name = mScreen->name();
    if (mScreen == qApp->primaryScreen()) {
        if (auto view = qobject_cast<DesktopIconView *>(centralWidget())) {
            // show desktop centralWidget
            this->show();
        }
    } else {
        if (mScreen->geometry() == qApp->primaryScreen()->geometry()) {
            this->hide();
        } else {
            this->show();
        }
    }

//    slotUpdateView();
    CT_SYSLOG(LOG_DEBUG, "end!");
}

void DesktopWindow::slotScaleBg(const QRect &geometry)
{
    CT_SYSLOG(LOG_DEBUG, "beginning ...");
    if (this->geometry() == geometry) {
        return;
    }

    setGeometry(geometry);
    //setWindowFlag(Qt::FramelessWindowHint, false);
    auto flags = windowFlags() &~Qt::WindowMinMaxButtonsHint;
    setWindowFlags(flags | Qt::FramelessWindowHint);

#if QT_VERSION_CHECK(5, 6, 0)
    Atom mWindowType = XInternAtom(QX11Info::display(), "_NET_WM_WINDOW_TYPE", true);
    Atom mDesktopType = XInternAtom(QX11Info::display(), "_NET_WM_WINDOW_TYPE_DESKTOP", true);
    XDeleteProperty(QX11Info::display(), winId(), mWindowType);
    XChangeProperty(QX11Info::display(), winId(), mWindowType, XA_ATOM, 32, 1, (unsigned char *)&mDesktopType, 1);
#endif
    show();
    mBgBackCachePixmap = mBgBackPixmap.scaled(geometry.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    mBgFontCachePixmap = mBgFontPixmap.scaled(geometry.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    this->update();
    CT_SYSLOG(LOG_DEBUG, "end!");
}

void DesktopWindow::slotSetBgPath(const QString &bgPath)
{
    CT_SYSLOG(LOG_DEBUG, "beginning ...");
    if (mBgSettings) {
        mBgSettings->set(PICTRUE, bgPath);
    } else {
        mBackupSetttings->setValue("pictrue", bgPath);
        mBackupSetttings->sync();
        Q_EMIT this->changeBg(bgPath);
    }
    CT_SYSLOG(LOG_DEBUG, "end!");
}

void DesktopWindow::slotGeometryChangedProcess(const QRect &geometry)
{
    slotUpdateWinGeometry();
}

void DesktopWindow::slotVirtualGeometryChangedProcess(const QRect &geometry)
{
    slotUpdateWinGeometry();
}

void DesktopWindow::slotAvailableGeometryChangedProcess(const QRect &geometry)
{
    slotUpdateView();
}

void DesktopWindow::paintEvent(QPaintEvent *e)
{
    QPainter p (this);
    p.setRenderHint(QPainter::Antialiasing);
    if (mOpacity->state() == QVariantAnimation::Running) {
        if (mUsePureColor) {
            auto opacity = mOpacity->currentValue().toDouble();
            p.fillRect(this->rect(), mLastPureColor);
            p.save();
            p.setOpacity(opacity);
            p.fillRect(this->rect(), mColorToBeSet);
            p.restore();
        } else {
            auto opacity = mOpacity->currentValue().toDouble();
            p.drawPixmap(this->rect(), mBgBackCachePixmap, mBgBackCachePixmap.rect());
            p.save();
            p.setOpacity(opacity);
            p.drawPixmap(this->rect(), mBgFontCachePixmap, mBgFontCachePixmap.rect());
            p.restore();
        }
    } else {
        if (mUsePureColor) {
            p.fillRect(this->rect(), mLastPureColor);
        } else {
            p.drawPixmap(this->rect(), mBgBackCachePixmap, mBgBackCachePixmap.rect());
        }
    }
    QMainWindow::paintEvent(e);
}

void DesktopWindow::slotSetBg(const QColor &color)
{
    CT_SYSLOG(LOG_DEBUG, "set background color: (%.2f, %.2f, %.2f, %.2f)", color.red(), color.green(), color.blue(), color.alpha());
    mColorToBeSet = color;
    mUsePureColor = true;

    if (mOpacity->state() == QVariantAnimation::Running) {
        mOpacity->setCurrentTime(500);
    } else {
        mOpacity->stop();
        mOpacity->start();
    }
    CT_SYSLOG(LOG_DEBUG, "end!");
}

void DesktopWindow::slotSetBg(const QString &bgPath)
{
    CT_SYSLOG(LOG_DEBUG, "background path:%s", bgPath.toUtf8().data());
    if (bgPath.isNull()) {
        CT_SYSLOG(LOG_WARNING, "current background image is not set!");
        slotSetBg(getCurrentColor());
        return;
    }

    mUsePureColor = false;
    mBgBackPixmap = mBgFontPixmap;
    mBgFontPixmap = QPixmap(bgPath);

    mBgBackCachePixmap = mBgBackPixmap.scaled(mScreen->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    mBgFontCachePixmap = mBgFontPixmap.scaled(mScreen->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    mCurrentBgPath = bgPath;
    slotSetBgPath(bgPath);

    if (mOpacity->state() == QVariantAnimation::Running) {
        mOpacity->setCurrentTime(500);
    } else {
        mOpacity->stop();
        mOpacity->start();
    }
    CT_SYSLOG(LOG_DEBUG, "set background end!");
}

void DesktopWindow::initShortcut()
{
    // 考虑放到 settings-daemon
}

void DesktopWindow::initGSettings()
{
    CT_SYSLOG(LOG_DEBUG, "beginning ...");
    if (!QGSettings::isSchemaInstalled(BACKGROUND_SETTINGS)) {
        mBackupSetttings = new QSettings ("org.graceful", "desktop", this);
        if (mBackupSetttings->value("color").isNull()) {
            auto defaultColor = QColor(Qt::cyan).darker();
            mBackupSetttings->setValue("color", defaultColor);
        }

        return;
    }

    mBgSettings = new QGSettings(BACKGROUND_SETTINGS, QByteArray(), this);

    connect(mBgSettings, &QGSettings::changed, this, [=](const QString &key) {
        CT_SYSLOG(LOG_DEBUG, "recive backgorund changed signal. key:%s", key.toUtf8().data());
        if (key == "pictureFilename") {
            auto bgPath = mBgSettings->get("pictureFilename").toString();
            if (!QFile::exists(bgPath)) {
                auto colorString = mBgSettings->get("primary-color").toString();
                auto color = QColor(colorString);
                CT_SYSLOG(LOG_DEBUG, "background picture is not exist, use pure color:%s", colorString.toUtf8().data());
                this->slotSetBg(color);
            } else {
                if (mCurrentBgPath == bgPath) {
                    return;
                }
                this->slotSetBg(bgPath);
            }
        }
        if (key == "primaryColor") {
            auto bgPath = mBgSettings->get("pictureFilename").toString();
            if (!bgPath.startsWith("/")) {
                auto colorString = mBgSettings->get("primary-color").toString();
                auto color = QColor(colorString);
                this->slotSetBg(color);
            }
        }
    });
    CT_SYSLOG(LOG_DEBUG, "init settings ok!");
}

DesktopIconView *DesktopWindow::getView()
{
    return mView;
}

void DesktopWindow::setScreen(QScreen *screen)
{
    mScreen = screen;
}

void DesktopWindow::gotoSetBackground ()
{
    CT_SYSLOG(LOG_DEBUG, "open control center for select background");
#if 0
    QProcess p;
    p.setProgram("控制面板");
    //old version use -a, new version use -b as para
    p.setArguments(QStringList()<<"-b");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    p.startDetached();
#else
    p.startDetached("ukui-control-center", QStringList()<<"-b");
#endif
    p.waitForFinished(-1);
#endif
}

QScreen *DesktopWindow::getScreen()
{
    return mScreen;
}

const QColor DesktopWindow::getCurrentColor()
{
    QColor color;
    if (mBgSettings) {
        color = qvariant_cast<QColor>(mBgSettings->get("primary-color"));
    } else {
        color = qvariant_cast<QColor>(mBackupSetttings->value("color"));
    }
    return color;
}

bool DesktopWindow::getIsPrimary()
{
    return mIsPrimary;
}
