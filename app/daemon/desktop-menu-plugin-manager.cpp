#include "desktop-menu-plugin-manager.h"

#include <QDir>
#include <QProxyStyle>
#include <QtConcurrent>
#include <QPluginLoader>
#include <QApplication>

#include <plugin-iface/style-plugin-iface.h>

static bool mIsLoading = false;
static DesktopMenuPluginManager *gInstance = nullptr;

DesktopMenuPluginManager *DesktopMenuPluginManager::getInstance()
{
    if (!gInstance) {
        if (!mIsLoading) {
            mIsLoading = true;
            gInstance = new DesktopMenuPluginManager;
            gInstance->loadAsync();
        }
    }
    return gInstance;
}

bool DesktopMenuPluginManager::isLoaded()
{
    return mIsLoaded;
}

const QStringList DesktopMenuPluginManager::getPluginIds()
{
    return mMap.keys();
}

QList<MenuPluginIface *> DesktopMenuPluginManager::getPlugins()
{
    return mMap.values();
}

MenuPluginIface *DesktopMenuPluginManager::getPlugin(const QString &pluginId)
{
    return mMap.value(pluginId);
}

void DesktopMenuPluginManager::loadAsync()
{
    QDir pluginsDir("/usr/lib/peony-qt-extensions/");
    pluginsDir.setFilter(QDir::Files);
    Q_FOREACH(QString fileName, pluginsDir.entryList(QDir::Files)) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (!plugin) {
            continue;
        }

        StylePluginIface *splugin = dynamic_cast<StylePluginIface*>(plugin);
        if (splugin) {
            QApplication::setStyle(splugin->getStyle());
            break;
        }
    }

    QtConcurrent::run([=]() {
        Q_FOREACH(QString fileName, pluginsDir.entryList(QDir::Files)) {
            QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
            pluginLoader.fileName();
            pluginLoader.metaData();
            pluginLoader.load();
            QObject *plugin = pluginLoader.instance();
            if (!plugin) {
                continue;
            }

            MenuPluginIface *piface = dynamic_cast<MenuPluginIface*>(plugin);
            if (!piface) {
                continue;
            }
            if (!mMap.value(piface->name())) {
                mMap.insert(piface->name(), piface);
            }
            mIsLoaded = true;
        }
    });
}

DesktopMenuPluginManager::DesktopMenuPluginManager(QObject *parent)
{
    mIsLoading = true;
    loadAsync();
}

DesktopMenuPluginManager::~DesktopMenuPluginManager()
{
    for (auto plugin : mMap) {
        delete plugin;
    }
    mMap.clear();
}
