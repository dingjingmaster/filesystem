#ifndef DESKTOPMENUPLUGINMANAGER_H
#define DESKTOPMENUPLUGINMANAGER_H

#include <QMap>
#include <QObject>
#include "plugin-iface/menu-plugin-iface.h"

class DesktopMenuPluginManager : public QObject
{
    Q_OBJECT
public:
    bool isLoaded();
    const QStringList getPluginIds();
    QList<MenuPluginIface*> getPlugins();
    static DesktopMenuPluginManager *getInstance();
    MenuPluginIface *getPlugin(const QString &pluginId);

protected:
    void loadAsync();

private:
    explicit DesktopMenuPluginManager(QObject *parent = nullptr);
    ~DesktopMenuPluginManager();

private:
    bool mIsLoaded = false;
    QMap<QString, MenuPluginIface*> mMap;
};

#endif // DESKTOPMENUPLUGINMANAGER_H
