#ifndef MENUPLUGINMANAGER_H
#define MENUPLUGINMANAGER_H

#include <QObject>

#include <plugin-iface/menu-plugin-iface.h>

class MenuPluginManager : public QObject
{
    Q_OBJECT
public:
    void close ();
    const QStringList getPluginIds ();
    static MenuPluginManager *getInstance ();
    bool registerPlugin (MenuPluginIface *plugin);
    MenuPluginIface *getPlugin (const QString &pluginId);

private:
    explicit MenuPluginManager (QObject *parent = nullptr);
    ~MenuPluginManager ();

private:
    QHash<QString, MenuPluginIface*> mHash;
};

#endif // MENUPLUGINMANAGER_H
