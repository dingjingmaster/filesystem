#include "properties-window.h"


PropertiesWindowPrivate::PropertiesWindowPrivate(const QStringList &uris, QWidget *parent) : QTabWidget(parent)
{

}

void PropertiesWindow::show()
{

}

void PropertiesWindow::gotoAboutComputer()
{

}

void PropertiesWindowPluginManager::release()
{

}

const QStringList PropertiesWindowPluginManager::getFactoryNames()
{

}

PropertiesWindowPluginManager *PropertiesWindowPluginManager::getInstance()
{

}

bool PropertiesWindowPluginManager::registerFactory(PropertiesWindowTabPagePluginIface *factory)
{

}

PropertiesWindowTabPagePluginIface *PropertiesWindowPluginManager::getFactory(const QString &id)
{

}

PropertiesWindowPluginManager::~PropertiesWindowPluginManager()
{

}

PropertiesWindowPluginManager::PropertiesWindowPluginManager(QObject *parent)
{

}

PropertiesWindow::PropertiesWindow(const QStringList &uris, QWidget *parent) : QMainWindow(parent)
{

}
