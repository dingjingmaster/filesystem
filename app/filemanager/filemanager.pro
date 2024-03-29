TEMPLATE = app

QT += core gui gui-private x11extras dbus concurrent widgets

TARGET = graceful-filemanager

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets widgets-private

PKGCONFIG +=                                        \
    gio-2.0                                         \
    glib-2.0                                        \
    poppler                                         \
    gio-unix-2.0                                    \

LIBS +=                                             \
    -lX11                                           \
    -lgio-2.0                                       \
    -lglib-2.0

CONFIG +=                                           \
    c++11                                           \
    no_keywords                                     \
    link_pkgconfig

INCLUDEPATH +=                                      \
    $$PWD/../                                       \
    $$PWD/../common                                 \
    $$PWD/../manager                                \
    $$PWD/../library                                \

SOURCES +=                                          \
    $$PWD/main.cpp                                  \

include($$PWD/../vfs/vfs.pri)
include($$PWD/../file/file.pri)
include($$PWD/../view/view.pri)
include($$PWD/../main/main.pri)
include($$PWD/../menu/menu.pri)
include($$PWD/../model/model.pri)
include($$PWD/../window/window.pri)
include($$PWD/../common/common.pri)
include($$PWD/../manager/manager.pri)
include($$PWD/../delegate/delegate.pri)
include($$PWD/../controls/controls.pri)
include($$PWD/../thumbnail/thumbnail.pri)
include($$PWD/../library/syslog/syslog.pri)
include($$PWD/../library/gobject/gobject.pri)
include($$PWD/../plugin-iface/plugin-iface.pri)
include($$PWD/../library/single-app/single-app.pri)
include($$PWD/../library/qgsettings/qgsettings-lib.pri)
include($$PWD/../library/kwindow-system/kwindow-system.pri)

RESOURCES +=
