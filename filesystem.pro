TEMPLATE = app

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets widgets-private

QMAKE_CFLAGS += -DLOG_LEVEL=7
QMAKE_CXXFLAGS += -DLOG_LEVEL=7

PKGCONFIG +=                                        \
    gio-2.0                                         \
    glib-2.0                                        \
    gio-unix-2.0

LIBS +=                                             \
    -lX11                                           \
    -lgio-2.0                                       \
    -lglib-2.0

CONFIG +=                                           \
    c++11                                           \
    no_keywords                                     \
    link_pkgconfig

INCLUDEPATH +=                                      \
    $$PWD/app                                       \
    $$PWD/app/common                                \
    $$PWD/app/library

SOURCES +=                                          \
    $$PWD/app/main.cpp                              \

OTHER_FILES +=                                      \
    $$PWD/LICENSE                                   \
    $$PWD/Doxyfile                                  \
    $$PWD/README.md                                 \
    $$PWD/.gitignore                                \
    $$PWD/translate/filesystem-manager.ts

include($$PWD/app/vfs/vfs.pri)
include($$PWD/app/file/file.pri)
include($$PWD/app/main/main.pri)
include($$PWD/app/model/model.pri)
include($$PWD/app/window/window.pri)
include($$PWD/app/common/common.pri)
include($$PWD/app/controls/controls.pri)
include($$PWD/app/library/syslog/syslog.pri)
include($$PWD/app/library/gobject/gobject.pri)
include($$PWD/app/library/single-app/single-app.pri)
include($$PWD/app/library/qgsettings/qgsettings-lib.pri)
include($$PWD/app/library/kwindow-system/kwindow-system.pri)
