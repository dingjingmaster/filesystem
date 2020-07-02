#ifndef PLUGINIFACE_H
#define PLUGINIFACE_H

#include <QIcon>
#include <QString>

class PluginIface
{
public:
    enum PluginType
    {
        Invalid,
        MenuPlugin,
        PreviewPagePlugin,
        DirectoryViewPlugin,
        DirectoryViewPlugin2,
        ToolBarPlugin,
        PropertiesWindowPlugin,
        ColumnProviderPlugin,
        StylePlugin,
        Other
    };

    virtual ~PluginIface () {}
    virtual bool isEnable () = 0;
    virtual const QIcon icon () = 0;
    virtual const QString name () = 0;
    virtual PluginType pluginType () = 0;
    virtual const QString description () = 0;
    virtual void setEnable (bool enable) = 0;
};

#endif // PLUGINIFACE_H
