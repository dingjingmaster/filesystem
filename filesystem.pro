TEMPLATE = app

QT += core gui

QMAKE_CFLAGS += -DLOG_LEVEL=7
QMAKE_CXXFLAGS += -DLOG_LEVEL=7

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets widgets-private

include($$PWD/app/vfs/vfs.pri)
include($$PWD/app/library/syslog/syslog.pri)
include($$PWD/app/library/single-app/single-app.pri)

PKGCONFIG += \
    gio-2.0 glib-2.0 gio-unix-2.0

LIBS += \
    -lgio-2.0 -lglib-2.0 -lX11

CONFIG += \
    c++11 link_pkgconfig no_keywords

INCLUDEPATH += \
    $$PWD/app/common/ \
    $$PWD/app/library/single-app \

SOURCES += \
    $$PWD/app/main.cpp \
    app/common/file-utils.cpp \
    app/filesyste-mmanager.cpp

OTHER_FILES += \
    $$PWD/Doxyfile \
    $$PWD/.gitignore \
    $$PWD/LICENSE \
    $$PWD/README.md

HEADERS += \
    app/common/file-utils.h \
    app/filesyste-mmanager.h
