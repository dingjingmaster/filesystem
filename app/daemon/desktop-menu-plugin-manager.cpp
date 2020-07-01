#include "desktop-menu-plugin-manager.h"


DesktopMenuPluginManager *DesktopMenuPluginManager::getInstance()
{

}

bool DesktopMenuPluginManager::isLoaded()
{
    return mIsLoaded;
}

const QStringList DesktopMenuPluginManager::getPluginIds()
{

}

QList<MenuPluginIface *> DesktopMenuPluginManager::getPlugins()
{

}

MenuPluginIface *DesktopMenuPluginManager::getPlugin(const QString &pluginId)
{

}

void DesktopMenuPluginManager::loadAsync()
{

}

DesktopMenuPluginManager::DesktopMenuPluginManager(QObject *parent)
{

}

DesktopMenuPluginManager::~DesktopMenuPluginManager()
{

}
