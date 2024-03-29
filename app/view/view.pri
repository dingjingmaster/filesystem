HEADERS += \
    $$PWD/border-shadow-effect.h \
    $$PWD/directory-view-container.h \
    $$PWD/directory-view-widget.h \
    $$PWD/icon-view-style.h \
    $$PWD/icon-view.h \
    $$PWD/list-view-style.h \
    $$PWD/list-view.h


SOURCES += \
    $$PWD/border-shadow-effect.cpp \
    $$PWD/directory-view-container.cpp \
    $$PWD/directory-view-widget.cpp \
    $$PWD/icon-view-style.cpp \
    $$PWD/icon-view.cpp \
    $$PWD/list-view-style.cpp \
    $$PWD/list-view.cpp

include($$PWD/view-factory/view-factory.pri)
