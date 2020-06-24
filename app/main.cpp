#include "filesyste-mmanager.h"

#include <QGuiApplication>
#include <stdio.h>

int main (int argc, char* argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    FilesystemManager fm (argc, argv);
    if (fm.isSecondary()) {
        return 0;
    }

    return fm.exec();
}
