#include "fm-desktop-application.h"

FMDesktopApplication::FMDesktopApplication(int &argc, char *argv[], const char *applicationName) : SingleApp(argc, argv, applicationName)
{

}

DesktopIconView *FMDesktopApplication::getIconView()
{

}

bool FMDesktopApplication::isPrimaryScreen(QScreen *screen)
{

}

void FMDesktopApplication::parseCmd(quint32 id, QByteArray msg, bool isPrimary)
{

}

void FMDesktopApplication::checkWindowProcess()
{

}

void FMDesktopApplication::screenAddedProcess(QScreen *screen)
{

}

void FMDesktopApplication::screenRemovedProcess(QScreen *screen)
{

}

void FMDesktopApplication::changeBgProcess(const QString &bgPath)
{

}

void FMDesktopApplication::primaryScreenChangedProcess(QScreen *screen)
{

}

void FMDesktopApplication::addWindow(QScreen *screen, bool checkPrimay)
{

}

void FMDesktopApplication::layoutDirectionChangedProcess(Qt::LayoutDirection direction)
{

}
