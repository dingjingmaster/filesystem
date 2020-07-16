INCLUDEPATH += \
    -I /usr/include/x86_64-linux-gnu/qt5    \
    -I /usr/include/x86_64-linux-gnu/qt5/QtWidgets/5.12.8/

HEADERS += \
    $$PWD/main-window.h                     \
    $$PWD/volume-manager.h                  \
    $$PWD/bookmark-manager.h                \
    $$PWD/properties-window.h               \
    $$PWD/main-window-factory.h             \


SOURCES += \
    $$PWD/main-window.cpp                   \
    $$PWD/volume-manager.cpp                \
    $$PWD/bookmark-manager.cpp              \
    $$PWD/properties-window.cpp             \
    $$PWD/main-window-factory.cpp           \
