#ifndef PROPERTIESWINDOWTABPAGEPLUGINIFACE_H
#define PROPERTIESWINDOWTABPAGEPLUGINIFACE_H

#include "plugin-iface.h"

class QWidget;

#define PropertiesWindowTabPagePluginIface_iid "org.graceful.fm.plugin-iface.PropertiesWindowTabPagePluginIface"

class PropertiesWindowTabPagePluginIface : public PluginIface
{
public:
    virtual ~PropertiesWindowTabPagePluginIface() {}

    virtual int tabOrder() = 0;
    virtual void closeFactory() = 0;
    virtual bool supportUris(const QStringList &uris) = 0;
    virtual QWidget *createTabPage(const QStringList &uris) = 0;
};

Q_DECLARE_INTERFACE(PropertiesWindowTabPagePluginIface, PropertiesWindowTabPagePluginIface_iid)


#endif // PROPERTIESWINDOWTABPAGEPLUGINIFACE_H
