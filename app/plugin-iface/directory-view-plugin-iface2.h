#ifndef DIRECTORYVIEWPLUGINIFACE2_H
#define DIRECTORYVIEWPLUGINIFACE2_H

#include "plugin-iface.h"

class QWidget;
class DirectoryViewWidget;

#define DirectoryViewPluginIface2_iid "org.graceful.fm.plugin-iface.DirectoryViewPluginIface2"

class DirectoryViewPluginIface2 : public PluginIface
{
public:
    virtual ~DirectoryViewPluginIface2() {}

    virtual QIcon viewIcon() = 0;
    virtual bool supportZoom() = 0;
    virtual QString viewName() = 0;
    virtual int zoom_level_hint() = 0;
    virtual QString viewIdentity() = 0;
    virtual DirectoryViewWidget *create() = 0;
    virtual int minimumSupportedZoomLevel() = 0;
    virtual int maximumSupportedZoomLevel() = 0;
    virtual bool supportUri(const QString &uri) = 0;
    virtual int priority(const QString &directoryUri) = 0;
};

Q_DECLARE_INTERFACE (DirectoryViewPluginIface2, DirectoryViewPluginIface2_iid)

#endif // DIRECTORYVIEWPLUGINIFACE2_H
