#include "desktop-application.h"

DesktopApplication::DesktopApplication(int &argc, char *argv[], const char *applicationName) : SingleApp(argc, argv, applicationName)
{

}

DesktopIconView *DesktopApplication::getIconView()
{

}

bool DesktopApplication::slotIsPrimaryScreen(QScreen *screen)
{

}

void DesktopApplication::slotParseCmd(quint32 id, QByteArray msg, bool isPrimary)
{

}

void DesktopApplication::slotScreenAddedProcess(QScreen *screen)
{

}

void DesktopApplication::slotScreenRemovedProcess(QScreen *screen)
{

}

void DesktopApplication::slotChangeBgProcess(const QString &bgPath)
{

}

void DesktopApplication::slotPrimaryScreenChangedProcess(QScreen *screen)
{

}

void DesktopApplication::slotAddWindow(QScreen *screen, bool checkPrimay)
{

}

void DesktopApplication::slotLayoutDirectionChangedProcess(Qt::LayoutDirection direction)
{

}
