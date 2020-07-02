#ifndef STYLEPLUGINIFACE_H
#define STYLEPLUGINIFACE_H

#include "plugin-iface.h"

class QString;
class QtPlugin;
class QProxyStyle;
class QPluginLoader;

#define StylePluginIface_iid "org.graceful.fm.plugin-iface.StylePluginIface"

class StylePluginIface : public PluginIface
{
public:
    ~StylePluginIface() {}

    virtual int defaultPriority() = 0;
    virtual QProxyStyle *getStyle() = 0;
};

Q_DECLARE_INTERFACE(StylePluginIface, StylePluginIface_iid)


#endif // STYLEPLUGINIFACE_H
