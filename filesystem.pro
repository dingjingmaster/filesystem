TEMPLATE = subdirs

CONFIG += debug_and_release
CONFIG(debug,debug|release) {
    QMAKE_CFLAGS += -DLOG_LEVEL=7
    QMAKE_CXXFLAGS += -DLOG_LEVEL=7
}

SUBDIRS = \
#    $$PWD/app/gui/gui.pro                           \
    $$PWD/app/daemon/daemon.pro                     \

OTHER_FILES +=                                      \
    $$PWD/LICENSE                                   \
    $$PWD/Doxyfile                                  \
    $$PWD/README.md                                 \
    $$PWD/.gitignore                                \
    $$PWD/data/graceful-desktop.desktop             \
    $$PWD/translate/filesystem-manager.ts           \
    $$PWD/data/graceful-desktop-home.desktop        \
    $$PWD/data/graceful-desktop-trash.desktop       \
    $$PWD/data/graceful-desktop-desktop.desktop     \
    $$PWD/data/graceful-desktop-computer.desktop    \

# 安装相关文件
data_graceful_desktop_icon.path = /usr/share/applications/
data_graceful_desktop_icon.files = $$PWD/data/graceful-desktop-*

INSTALLS += \
    data_graceful_desktop_icon
