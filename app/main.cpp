#include <QGuiApplication>
#include <stdio.h>

int main (int argc, char* argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);


    return 0;
}
