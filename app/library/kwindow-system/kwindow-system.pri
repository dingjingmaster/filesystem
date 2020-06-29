QT += x11extras

LIBS += \
    -lxcb

INCLUDEPATH += \
    $$PWD/                                  \
    $$PWD/platforms/xcb                     \

SOURCES += \
    $$PWD/kkeyserver.cpp                    \
    $$PWD/kwindowinfo.cpp                   \
    $$PWD/pluginwrapper.cpp                 \
    $$PWD/kwindowshadow.cpp                 \
    $$PWD/kwindowsystem.cpp                 \
    $$PWD/kwindoweffects.cpp                \
    $$PWD/kwindowsystem_debug.cpp           \
    $$PWD/kwindoweffects_dummy.cpp          \
    $$PWD/kwindowsystemplugininterface.cpp  \

HEADERS += \
    $$PWD/netwm_def.h                       \
    $$PWD/kkeyserver.h                      \
    $$PWD/kwindowinfo.h                     \
    $$PWD/kwindowshadow.h                   \
    $$PWD/kwindowsystem.h                   \
    $$PWD/kwindowinfo_p.h                   \
    $$PWD/kwindoweffects.h                  \
    $$PWD/kwindowsystem_p.h                 \
    $$PWD/pluginwrapper_p.h                 \
    $$PWD/kwindowshadow_p.h                 \
    $$PWD/kwindowinfo_dummy_p.h             \
    $$PWD/kwindowsystem_debug.h             \
    $$PWD/kwindowsystem_export.h            \
    $$PWD/config-kwindowsystem.h            \
    $$PWD/kwindowsystem_dummy_p.h           \
    $$PWD/kwindowshadow_dummy_p.h           \
    $$PWD/kwindoweffects_dummy_p.h          \
    $$PWD/platforms/xcb/kkeyserver_x11.h    \
    $$PWD/kwindowsystemplugininterface_p.h  \
