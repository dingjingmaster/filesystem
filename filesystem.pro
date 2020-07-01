TEMPLATE = subdirs

CONFIG += debug_and_release
CONFIG(debug,debug|release) {
    QMAKE_CFLAGS += -DLOG_LEVEL=7
    QMAKE_CXXFLAGS += -DLOG_LEVEL=7
}

SUBDIRS = \
    $$PWD/app/widget/gui.pro                        \
    $$PWD/app/daemon/daemon.pro                     \

OTHER_FILES +=                                      \
    $$PWD/LICENSE                                   \
    $$PWD/Doxyfile                                  \
    $$PWD/README.md                                 \
    $$PWD/.gitignore                                \
    $$PWD/translate/filesystem-manager.ts

