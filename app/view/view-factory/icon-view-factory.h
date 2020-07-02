#ifndef ICONVIEWFACTORY_H
#define ICONVIEWFACTORY_H

#include <QObject>
#include "plugin-iface/directory-view-plugin-iface.h"
#include "plugin-iface/directory-view-plugin-iface2.h"

class IconViewFactory : public QObject, public DirectoryViewPluginIface
{
    Q_OBJECT
public:
    static IconViewFactory *getInstance();

    bool isEnable() override;
    QIcon viewIcon() override;
    const QIcon icon() override;
    const QString name() override;
    int zoom_level_hint() override;
    QString viewIdentity() override;
    PluginType pluginType() override;
    const QString description() override;
    void setEnable(bool enable) override;
    DirectoryViewIface *create() override;
    int priority(const QString &) override;
    bool supportUri(const QString &uri) override;

private:
    explicit IconViewFactory(QObject *parent = nullptr);
    ~IconViewFactory() override;
};

class IconViewFactory2 : public QObject, public DirectoryViewPluginIface2
{
    Q_OBJECT
public:
    static IconViewFactory2 *getInstance();

    bool isEnable() override;
    QIcon viewIcon() override;
    QString viewName() override;
    const QIcon icon() override;
    bool supportZoom() override;
    const QString name() override;
    int zoom_level_hint() override;
    QString viewIdentity() override;
    PluginType pluginType() override;
    const QString description() override;
    void setEnable(bool enable) override;
    DirectoryViewWidget *create() override;
    int priority(const QString &) override;
    int minimumSupportedZoomLevel() override;
    int maximumSupportedZoomLevel() override;
    bool supportUri(const QString &uri) override;

private:
    explicit IconViewFactory2(QObject *parent = nullptr);
    ~IconViewFactory2() override;
};

#endif // ICONVIEWFACTORY_H
