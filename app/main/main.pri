HEADERS += \
    $$PWD/fm-window.h                           \
    $$PWD/fm-window-iface.h                     \
    $$PWD/fm-window-factory.h                   \
    $$PWD/filesystem-manager.h                  \
    $$PWD/prop-window-tabpage-plugin-iface.h \
    $$PWD/x11-window-manager.h

SOURCES += \
    $$PWD/fm-window.cpp                         \
    $$PWD/fm-window-iface.cpp                   \
    $$PWD/fm-window-factory.cpp                 \
    $$PWD/filesystem-manager.cpp                \
    $$PWD/prop-window-tabpage-plugin-iface.cpp \
    $$PWD/x11-window-manager.cpp

RESOURCES += \
    $$PWD/desktop-style.qrc
