CONFIG += qt no_install_prl c++11

PKGCONFIG +=                    \
    gio-2.0                     \
    glib-2.0                    \

HEADERS +=                      \
    $$PWD/qconftype.h           \
    $$PWD/qgsettings.h

SOURCES +=                      \
    $$PWD/qconftype.cpp         \
    $$PWD/qgsettings.cpp
