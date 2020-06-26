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
    $$PWD/app/library                               \
    $$PWD/app/file-operation                        \

SOURCES +=                                          \
    $$PWD/app/main.cpp                              \
    $$PWD/app/filesyste-mmanager.cpp

OTHER_FILES +=                                      \
    $$PWD/LICENSE                                   \
    $$PWD/Doxyfile                                  \
    $$PWD/README.md                                 \
    $$PWD/.gitignore

HEADERS +=                                          \
    $$PWD/app/common/file-utils.h                   \
    $$PWD/app/filesyste-mmanager.h

include($$PWD/app/vfs/vfs.pri)
include($$PWD/app/common/common.pri)
include($$PWD/app/library/syslog/syslog.pri)
include($$PWD/app/library/gobject/gobject.pri)
include($$PWD/app/file-operation/file-operation.pri)
include($$PWD/app/library/single-app/single-app.pri)
