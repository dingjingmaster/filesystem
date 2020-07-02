#ifndef DIRECTORYVIEWPLUGINIFACE_H
#define DIRECTORYVIEWPLUGINIFACE_H

#include "plugin-iface.h"

#include <QObject>

#define DirectoryViewPluginIface_iid "org.graceful.fm.plugin-iface.DirectoryViewPluginIface"

class FileItemModel;
class DirectoryViewIface;
class DirectoryViewProxyIface;
class FileItemProxyFilterSortModel;

class DirectoryViewPluginIface : public PluginIface
{
public:
    virtual ~DirectoryViewPluginIface() {}

    virtual QIcon viewIcon() = 0;
    virtual int zoom_level_hint() = 0;
    virtual QString viewIdentity() = 0;
    virtual DirectoryViewIface *create() = 0;
    virtual bool supportUri(const QString &uri) = 0;
    virtual int priority(const QString &directoryUri) = 0;

};

class DirectoryViewIface
{
public:
    virtual ~DirectoryViewIface() {}

    virtual void closeView() = 0;
    virtual int getSortType() = 0;
    virtual int getSortOrder() = 0;
    const virtual QString viewId() = 0;
    virtual void invertSelections() = 0;
    virtual void stopLocationChange() = 0;
    virtual void beginLocationChange() = 0;
    virtual void setSortType(int sortType) = 0;
    const virtual QString getDirectoryUri() = 0;
    virtual void setSortOrder(int sortOrder) = 0;
    virtual void editUri(const QString &uri) = 0;
    const virtual QStringList getSelections() = 0;
    const virtual QStringList getAllFileUris() = 0;
    virtual DirectoryViewProxyIface *getProxy() = 0;
    virtual void editUris(const QStringList uris) = 0;
    virtual void setDirectoryUri(const QString &uri) = 0;
    virtual void setCutFiles(const QStringList &uris) = 0;
    virtual void scrollToSelection(const QString &uri) = 0;
    virtual void setSelections(const QStringList &uris) = 0;
    virtual void setProxy(DirectoryViewProxyIface *proxy) = 0;
    virtual void open(const QStringList &uris, bool newWindow) = 0;
    virtual void bindModel(FileItemModel *sourceModel, FileItemProxyFilterSortModel *proxyModel) = 0;
};

class DirectoryViewProxyIface: public QObject
{
    Q_OBJECT
public:
    explicit DirectoryViewProxyIface(QObject *parent = nullptr) : QObject(parent) {}
    ~DirectoryViewProxyIface() {}

    virtual DirectoryViewIface *getView() = 0;
    const virtual QString getDirectoryUri() = 0;
    const virtual QStringList getSelections() = 0;
    const virtual QStringList getAllFileUris() = 0;
    virtual void switchView(DirectoryViewIface *view) = 0;

Q_SIGNALS:

    virtual int getSortType() = 0;
    virtual int getSortOrder() = 0;

    void viewDirectoryChanged();
    void viewSelectionChanged();
    void menuRequest(const QPoint &pos);
    void viewDoubleClicked(const QString &uri);
    void updateWindowLocationRequest(const QString &uri);
    void openRequest(const QStringList &uri, bool newWindow);

public Q_SLOTS:
    virtual void closeProxy() = 0;
    virtual void invertSelections() = 0;
    virtual void stopLocationChange() = 0;
    virtual void beginLocationChange() = 0;
    virtual void setSortType(int sortType) = 0;
    virtual void setSortOrder(int sortOrder) = 0;
    virtual void editUri(const QString &uri) = 0;
    virtual void editUris(const QStringList uris) = 0;
    virtual void setDirectoryUri(const QString &uri) = 0;
    virtual void setCutFiles(const QStringList &uris) = 0;
    virtual void scrollToSelection(const QString &uri) = 0;
    virtual void setSelections(const QStringList &uris) = 0;
    virtual void open(const QStringList &uris, bool newWindow) = 0;
};

Q_DECLARE_INTERFACE(DirectoryViewPluginIface, DirectoryViewPluginIface_iid)

#endif // DIRECTORYVIEWPLUGINIFACE_H
