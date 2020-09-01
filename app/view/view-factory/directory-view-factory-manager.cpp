#include "directory-view-factory-manager.h"

#include <QDebug>
#include <QSettings>

#include "icon-view-factory.h"
#include "list-view-factory.h"
#include "plugin-iface/directory-view-plugin-iface.h"

static DirectoryViewFactoryManager2 *globalInstance2 = nullptr;

DirectoryViewFactoryManager2* DirectoryViewFactoryManager2::getInstance()
{
    if (!globalInstance2) {
        globalInstance2 = new DirectoryViewFactoryManager2;
    }
    return globalInstance2;
}

DirectoryViewFactoryManager2::DirectoryViewFactoryManager2(QObject *parent) : QObject(parent)
{
    mSettings = new QSettings("Graceful Linux", "Filesystem manager", this);
    m_hash = new QHash<QString, DirectoryViewPluginIface2*>();

    //register icon view and list view
//    auto iconViewFactory2 = IconViewFactory2::getInstance();
//    registerFactory(iconViewFactory2->viewIdentity(), iconViewFactory2);
//    mInternalViews << "Icon View";

//    auto listViewFactory2 = ListViewFactory2::getInstance();
//    registerFactory(listViewFactory2->viewIdentity(), listViewFactory2);
//    mInternalViews << "List View";
}

DirectoryViewFactoryManager2::~DirectoryViewFactoryManager2()
{

}

void DirectoryViewFactoryManager2::registerFactory(const QString &name, DirectoryViewPluginIface2 *factory)
{
    if (m_hash->value(name)) {
        return;
    }
    m_hash->insert(name, factory);
}

QStringList DirectoryViewFactoryManager2::getFactoryNames()
{
    return m_hash->keys();
}

DirectoryViewPluginIface2 *DirectoryViewFactoryManager2::getFactory(const QString &name)
{
    return m_hash->value(name);
}

const QString DirectoryViewFactoryManager2::getDefaultViewId(const QString &uri)
{
    if (mDefaultViewIdCache.isNull()) {
        auto string = mSettings->value("directory-view/default-view-id").toString();
        if (string.isEmpty()) {
            string = "Icon View";
        } else {
            if (!m_hash->contains(string))
                string = "Icon View";
        }
        mDefaultViewIdCache = string;
    }
    return mDefaultViewIdCache;
}

const QString DirectoryViewFactoryManager2::getDefaultViewId(int zoomLevel, const QString &uri)
{
    auto factorys = m_hash->values();

    auto defaultFactory = getFactory(getDefaultViewId());
    int priorty = 0;

    for (auto factory : factorys) {
        if (factory->supportUri(uri)) {
            if (factory->priority(uri) > priorty) {
                defaultFactory = factory;
                priorty = factory->priority(uri);
                continue;
            }
        }
    }

    if (!defaultFactory->supportZoom())
        return defaultFactory->viewIdentity();

    if (zoomLevel < 0)
        return getDefaultViewId(uri);

    if (defaultFactory->supportZoom()) {
        if (zoomLevel <= 20 && zoomLevel >=0) {
            defaultFactory = getFactory("List View");
        }
        if (zoomLevel >= 21) {
            defaultFactory = getFactory("Icon View");
        }
    }

    return defaultFactory->viewIdentity();
}

void DirectoryViewFactoryManager2::setDefaultViewId(const QString &viewId)
{
    if (!mInternalViews.contains(viewId))
        return;

    if (getFactoryNames().contains(viewId)) {
        mDefaultViewIdCache = viewId;
    }
}

void DirectoryViewFactoryManager2::saveDefaultViewOption()
{
    mSettings->setValue("directory-view/default-view-id", mDefaultViewIdCache);
}
