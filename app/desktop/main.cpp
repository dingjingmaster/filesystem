#include "fm-desktop-application.h"

#include <clib_syslog.h>
#include <QGuiApplication>

int main (int argc, char* argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    FMDesktopApplication a(argc, argv);
    if (a.isSecondary()) {
        CT_SYSLOG(LOG_DEBUG, "FMDesktopApplication is secondary, exit!");
        return 0;
    }

    return a.exec();
}
