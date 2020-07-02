#ifndef LISTVIEWFACTORY_H
#define LISTVIEWFACTORY_H

#include <plugin-iface/directory-view-plugin-iface.h>
#include <plugin-iface/directory-view-plugin-iface2.h>

class ListViewFactory : public QObject, public DirectoryViewPluginIface
{
    Q_OBJECT
public:
    static ListViewFactory* getInstance();

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
    explicit ListViewFactory(QObject *parent = nullptr);
    ~ListViewFactory() override;
};

class ListViewFactory2 : public QObject, public DirectoryViewPluginIface2
{
    Q_OBJECT
public:
    static ListViewFactory2* getInstance();

    bool isEnable() override;
    QIcon viewIcon() override;
    const QIcon icon() override;
    QString viewName() override;
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
    explicit ListViewFactory2(QObject *parent = nullptr);
    ~ListViewFactory2() override;
};

#endif // LISTVIEWFACTORY_H
