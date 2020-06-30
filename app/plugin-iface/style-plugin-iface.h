#ifndef STYLEPLUGINIFACE_H
#define STYLEPLUGINIFACE_H

#include <QString>
#include <QtPlugin>
#include <QProxyStyle>
#include <QPluginLoader>

#include "plugin-iface.h"

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
