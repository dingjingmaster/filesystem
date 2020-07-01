TEMPLATE = app

QT       += core gui x11extras dbus concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = fm-daemon


HEADERS += \


SOURCES += \
    $$PWD/main.cpp

