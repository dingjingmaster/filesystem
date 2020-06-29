#include "main/filesystem-manager.h"

#include <stdio.h>
#include <QGuiApplication>

int main (int argc, char* argv[])
{
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    FilesystemManager fm (argc, argv);
    if (fm.isSecondary()) {
        return 0;
    }

    return fm.exec();
}
