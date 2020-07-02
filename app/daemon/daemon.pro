TEMPLATE = app

QT       += core gui x11extras dbus concurrent network

TARGET = fm-daemon

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG +=                                           \
    c++11                                           \
    no_keywords                                     \
    link_pkgconfig

INCLUDEPATH +=                                      \
    $$PWD/../                                       \
    $$PWD/../common                                 \
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
    $$PWD/desktop-icon-view.h                       \
    $$PWD/desktop-item-model.h                      \
    $$PWD/desktop-application.h                     \
    $$PWD/desktop-index-widget.h                    \
    $$PWD/fm-desktop-application.h                  \
    $$PWD/desktop-item-proxy-model.h                \
    $$PWD/desktop-icon-view-delegate.h              \
    $$PWD/desktop-menu-plugin-manager.h             \


SOURCES +=                                          \
    $$PWD/main.cpp                                  \
    $$PWD/desktop-menu.cpp                          \
    $$PWD/desktop-window.cpp                        \
    $$PWD/desktop-icon-view.cpp                     \
    $$PWD/desktop-item-model.cpp                    \
    $$PWD/desktop-application.cpp                   \
    $$PWD/desktop-index-widget.cpp                  \
    $$PWD/fm-desktop-application.cpp                \
    $$PWD/desktop-item-proxy-model.cpp              \
    $$PWD/desktop-icon-view-delegate.cpp            \
    $$PWD/desktop-menu-plugin-manager.cpp           \


include($$PWD/../library/syslog/syslog.pri)
include($$PWD/../library/single-app/single-app.pri)
include($$PWD/../library/qgsettings/qgsettings-lib.pri)
