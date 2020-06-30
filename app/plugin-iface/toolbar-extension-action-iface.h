#ifndef TOOLBAREXTENSIONACTIONIFACE_H
#define TOOLBAREXTENSIONACTIONIFACE_H

#include "plugin-iface.h"

#include <QAction>


#define ToolBarActionPluginIface_iid "org.graceful.fm.plugin-iface.ToolBarActionPluginIface"

class ToolBarExtensionActionIface : public QAction
{
    Q_OBJECT
    ~ToolBarExtensionActionIface();

public Q_SLOTS:
    virtual void excuteAction(const QString &directoryUri, const QStringList &selectedUris) = 0;
};

class ToolBarActionPluginIface : public PluginIface
{

public:
    ~ToolBarActionPluginIface();

    virtual ToolBarExtensionActionIface *create() = 0;
};

Q_DECLARE_INTERFACE(ToolBarActionPluginIface, ToolBarActionPluginIface_iid)

#endif // TOOLBAREXTENSIONACTIONIFACE_H
