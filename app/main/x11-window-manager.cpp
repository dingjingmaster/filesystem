#include "x11-window-manager.h"

#include <QStyle>
#include <QTabBar>
#include <QWidget>
#include <QMouseEvent>
#include <QApplication>
#include <QStyleOptionTabBarBase>


#include <QX11Info>
#include <X11/Xlib.h>

static X11WindowManager *gInstance = nullptr;

X11WindowManager *X11WindowManager::getInstance()
{
    if (!gInstance) {
        gInstance = new X11WindowManager;
    }
    return gInstance;
}

X11WindowManager::X11WindowManager(QObject *parent) : QObject(parent)
{

}

bool X11WindowManager::eventFilter(QObject *watched, QEvent *event)
{
    //qDebug()<<event->type();

    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchCancel:
    case QEvent::TouchEnd: {
        break;
    }

    case QEvent::MouseButtonPress: {

        //Make tabbar blank area dragable and do not effect tablabel.
        if (qobject_cast<QTabBar *>(watched)) {
            QMouseEvent *e = static_cast<QMouseEvent *>(event);
            QTabBar *tabBar = qobject_cast<QTabBar *>(watched);
            if (tabBar->tabAt(e->pos()) != -1)
                return false;
        }
        QMouseEvent *e = static_cast<QMouseEvent *>(event);

        if (QObject::eventFilter(watched, event))
            return true;
        if (e->button() == Qt::LeftButton) {
            mPressPos = QCursor::pos();
            mIsDraging = true;
            mCurrentWidget = static_cast<QWidget *>(watched);
            mToplevelOffset = mCurrentWidget->topLevelWidget()->mapFromGlobal(mPressPos);
        }

        //qDebug()<<event->type();

        break;
    }
    case QEvent::MouseMove: {
        QMouseEvent *e = static_cast<QMouseEvent *>(event);

        bool isTouchMove = e->source() == Qt::MouseEventSynthesizedByQt;

        if (mIsDraging) {
            if (QX11Info::isPlatformX11()) {
                Display *display = QX11Info::display();
                Atom netMoveResize = XInternAtom(display, "_NET_WM_MOVERESIZE", False);
                XEvent xEvent;
                const auto pos = QCursor::pos();

                memset(&xEvent, 0, sizeof(XEvent));
                xEvent.xclient.type = ClientMessage;
                xEvent.xclient.message_type = netMoveResize;
                xEvent.xclient.display = display;
                xEvent.xclient.window = mCurrentWidget->topLevelWidget()->winId();
                xEvent.xclient.format = 32;
                xEvent.xclient.data.l[0] = pos.x() * qApp->devicePixelRatio();
                xEvent.xclient.data.l[1] = pos.y() * qApp->devicePixelRatio();
                xEvent.xclient.data.l[2] = 8;
                xEvent.xclient.data.l[3] = Button1;
                xEvent.xclient.data.l[4] = 0;

                XUngrabPointer(display, CurrentTime);
                XSendEvent(display, QX11Info::appRootWindow(QX11Info::appScreen()),
                           False, SubstructureNotifyMask | SubstructureRedirectMask,
                           &xEvent);
                XFlush(display);

                mIsDraging = false;

                //NOTE: use x11 move will ungrab the window focus
                //hide and show will restore the focus and it seems
                //there is no bad effect for peony main window.
                if (isTouchMove) {
                    if (!mCurrentWidget->mouseGrabber()) {
                        mCurrentWidget->grabMouse();
                        mCurrentWidget->releaseMouse();
                    }
                }

//                if (qobject_cast<NavigationTabBar *>(mCurrentWidget)) {
//                    mCurrentWidget->hide();
//                    mCurrentWidget->show();
//                }

                //balance mouse release event
                QMouseEvent me(QMouseEvent::MouseButtonRelease, e->pos(), e->windowPos(), e->screenPos(), e->button(), e->buttons(), e->modifiers(), Qt::MouseEventSynthesizedByApplication);
                qApp->sendEvent(watched, &me);

                return true;
            } else {
                //auto me = static_cast<QMouseEvent *>(event);
                auto widget = qobject_cast<QWidget *>(watched);
                auto topLevel = widget->topLevelWidget();
                auto globalPos = QCursor::pos();
                //auto offset = globalPos - mPressPos;
                topLevel->move(globalPos - mToplevelOffset);
            }
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        auto me = static_cast<QMouseEvent *>(event);

        if (me->source() == Qt::MouseEventSynthesizedByApplication)
            break;

        mPressPos = QPoint();
        mIsDraging = false;
        mCurrentWidget = nullptr;
        break;
    }
    default:
        return false;
    }
    return false;
}

void X11WindowManager::registerWidget(QWidget *widget)
{
    widget->removeEventFilter(this);
    widget->installEventFilter(this);
}
