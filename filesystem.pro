TEMPLATE = subdirs

export("QT_SELECT=5")

CONFIG += \
    c++11                                                   \
    no_keywords                                             \
    link_pkgconfig                                          \
    debug_and_release                                       \

CONFIG(debug,debug|release) {
    QMAKE_CFLAGS += -DLOG_LEVEL=7
    QMAKE_CXXFLAGS += -DLOG_LEVEL=7
}

SUBDIRS = \
    $$PWD/app/desktop/desktop.pro                           \
    $$PWD/app/filemanager/filemanager.pro                   \

OTHER_FILES += \
    $$PWD/LICENSE                                           \
    $$PWD/Doxyfile                                          \
    $$PWD/README.md                                         \
    $$PWD/.gitignore                                        \
    $$PWD/translate/filesystem-manager.ts                   \
    $$PWD/data/graceful-filesystem.desktop                  \
    $$PWD/data/graceful-filesystem-home.desktop             \
    $$PWD/data/graceful-filesystem-trash.desktop            \
    $$PWD/data/graceful-filesystem-computer.desktop         \
    $$PWD/data/org.graceful.desktop.gschema.xml             \

# 安装相关文件
data_graceful_filesystem_icon.path = /usr/share/applications/
data_graceful_filesystem_icon.files = $$PWD/data/graceful-filemanager-*

# 安装gsettings schema文件
data_graceful_desktop_schema.path = /usr/share/glib-2.0/schemas/
data_graceful_desktop_schema.files = $$PWD/data/org.graceful.*.xml

# install graceful-desktop
bin_graceful_desktop.path = /usr/bin/
bin_graceful_desktop.files = $$PWD/app/desktop/graceful-desktop

# install graceful-filemanager
bin_graceful_filemanager.path = /usr/bin/
bin_graceful_filemanager.files = $$PWD/app/filemanager/graceful-filemanager

# install binary
INSTALLS += \
    bin_graceful_desktop                                    \
    bin_graceful_filemanager                                \

# install data
INSTALLS += \
    data_graceful_desktop_schema                            \
    data_graceful_filesystem_icon                           \
