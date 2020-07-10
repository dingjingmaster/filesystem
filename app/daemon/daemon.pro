TEMPLATE = app

QT       += core gui x11extras dbus concurrent network

TARGET = graceful-desktop

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS += -std=c++0x

CONFIG +=                                           \
    c++11                                           \
    no_keywords                                     \
    link_pkgconfig

INCLUDEPATH +=                                      \
    $$PWD/../                                       \
    $$PWD/../common                                 \
    $$PWD/../manager                                \
    $$PWD/../library

PKGCONFIG +=                                        \
    gio-2.0                                         \
    glib-2.0                                        \
    gio-unix-2.0

LIBS +=                                             \
    -lX11                                           \
    -lgio-2.0                                       \
    -lglib-2.0

HEADERS +=                                          \
    $$PWD/desktop-menu.h                            \
    $$PWD/desktop-window.h                          \
    $$PWD/fm-dbus-service.h                         \
    $$PWD/desktop-icon-view.h                       \
    $$PWD/desktop-item-model.h                      \
    $$PWD/desktop-index-widget.h                    \
    $$PWD/fm-desktop-application.h                  \
    $$PWD/desktop-item-proxy-model.h                \
    $$PWD/desktop-icon-view-delegate.h              \
    $$PWD/desktop-menu-plugin-manager.h

SOURCES +=                                          \
    $$PWD/main.cpp                                  \
    $$PWD/desktop-menu.cpp                          \
    $$PWD/desktop-window.cpp                        \
    $$PWD/fm-dbus-service.cpp                       \
    $$PWD/desktop-icon-view.cpp                     \
    $$PWD/desktop-item-model.cpp                    \
    $$PWD/desktop-index-widget.cpp                  \
    $$PWD/fm-desktop-application.cpp                \
    $$PWD/desktop-item-proxy-model.cpp              \
    $$PWD/desktop-icon-view-delegate.cpp            \
    $$PWD/desktop-menu-plugin-manager.cpp

OTHER_FILES +=                                      \
    $$PWD/freedesktop-dbus-iface.xml                \
    $$PWD/org.graceful.freedesktop.fm.service

include($$PWD/../vfs/vfs.pri)
include($$PWD/../file/file.pri)
include($$PWD/../view/view.pri)
include($$PWD/../main/main.pri)
include($$PWD/../model/model.pri)
include($$PWD/../window/window.pri)
include($$PWD/../common/common.pri)
include($$PWD/../dialog/dialog.pri)
include($$PWD/../manager/manager.pri)
include($$PWD/../controls/controls.pri)
include($$PWD/../delegate/delegate.pri)
include($$PWD/../thumbnail/thumbnail.pri)
include($$PWD/../library/syslog/syslog.pri)
include($$PWD/../library/gobject/gobject.pri)
include($$PWD/../plugin-iface/plugin-iface.pri)
include($$PWD/../library/single-app/single-app.pri)
include($$PWD/../library/qgsettings/qgsettings-lib.pri)
include($$PWD/../library/kwindow-system/kwindow-system.pri)

DISTFILES += \
    README.md
