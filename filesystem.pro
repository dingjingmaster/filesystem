TEMPLATE = app

QT += core gui

QMAKE_CFLAGS += -DLOG_LEVEL=7
QMAKE_CXXFLAGS += -DLOG_LEVEL=7

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets widgets-private

include($$PWD/app/library/single-app/single-app.pri)
include($$PWD/app/library/syslog/syslog.pri)

INCLUDEPATH += \
    $$PWD/app/library/single-app

SOURCES += \
    $$PWD/app/main.cpp \
    app/filesyste-mmanager.cpp

OTHER_FILES += \
    $$PWD/Doxyfile \
    $$PWD/.gitignore \
    $$PWD/LICENSE \
    $$PWD/README.md

HEADERS += \
    app/filesyste-mmanager.h
