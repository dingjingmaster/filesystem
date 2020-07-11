TEMPLATE = subdirs

CONFIG += debug_and_release
CONFIG(debug,debug|release) {
    QMAKE_CFLAGS += -DLOG_LEVEL=7
    QMAKE_CXXFLAGS += -DLOG_LEVEL=7
}

SUBDIRS =                                                   \
    $$PWD/app/desktop/desktop.pro                           \
    $$PWD/app/filemanager/filemanager.pro                   \

OTHER_FILES +=                                              \
    $$PWD/LICENSE                                           \
    $$PWD/Doxyfile                                          \
    $$PWD/README.md                                         \
    $$PWD/.gitignore                                        \
    $$PWD/data/graceful-desktop.desktop                     \
    $$PWD/translate/filesystem-manager.ts                   \
    $$PWD/data/graceful-desktop-home.desktop                \
    $$PWD/data/graceful-desktop-trash.desktop               \
    $$PWD/data/graceful-desktop-desktop.desktop             \
    $$PWD/data/graceful-desktop-computer.desktop            \
    $$PWD/data/org.graceful.desktop.gschema.xml             \

# 安装相关文件
data_graceful_desktop_icon.path = /usr/share/applications/
data_graceful_desktop_icon.files = $$PWD/data/graceful-desktop-*

# 安装gsettings schema文件
data_graceful_desktop_schema.path = /usr/share/glib-2.0/schemas/
data_graceful_desktop_schema.files = $$PWD/data/org.graceful.*.xml

INSTALLS +=                                                 \
    data_graceful_desktop_icon                              \
    data_graceful_desktop_schema
