TEMPLATE = app

QMAKE_CFLAGS += -DLOG_LEVEL=7
QMAKE_CXXFLAGS += -DLOG_LEVEL=7

include($$PWD/app/library/single-app/single-app.pri)
include($$PWD/app/library/syslog/syslog.pri)

SOURCES += \
#    $$PWD/app/main.cpp

OTHER_FILES += \
    $$PWD/Doxyfile \
    $$PWD/.gitignore \
    $$PWD/LICENSE \
    $$PWD/README.md
