#include "directory-view-factory-manager2.h"

#include <global-settings.h>

#include <view-factory/icon-view-factory.h>
#include <view-factory/list-view-factory.h>

static DirectoryViewFactoryManager2 *gInstance2 = nullptr;

DirectoryViewFactoryManager2* DirectoryViewFactoryManager2::getInstance()
{
    if (!gInstance2) {
        gInstance2 = new DirectoryViewFactoryManager2;
    }
    return gInstance2;
}

DirectoryViewFactoryManager2::DirectoryViewFactoryManager2(QObject *parent) : QObject(parent)
{
    mSettings = GlobalSettings::getInstance();
    mHash = new QHash<QString, DirectoryViewPluginIface2*>();

    //register icon view and list view
    auto iconViewFactory2 = IconViewFactory2::getInstance();
    registerFactory(iconViewFactory2->viewIdentity(), iconViewFactory2);
    mInternalViews << "Icon View";

    auto listViewFactory2 = ListViewFactory2::getInstance();
    registerFactory(listViewFactory2->viewIdentity(), listViewFactory2);
    mInternalViews << "List View";
}

DirectoryViewFactoryManager2::~DirectoryViewFactoryManager2()
{

}

void DirectoryViewFactoryManager2::registerFactory(const QString &name, DirectoryViewPluginIface2 *factory)
{
    if (mHash->value(name)) {
        return;
    }
    mHash->insert(name, factory);
}

QStringList DirectoryViewFactoryManager2::getFactoryNames()
{
    return mHash->keys();
}

DirectoryViewPluginIface2 *DirectoryViewFactoryManager2::getFactory(const QString &name)
{
    return mHash->value(name);
}

const QString DirectoryViewFactoryManager2::getDefaultViewId(const QString &uri)
{
    if (mDefaultViewIdCache.isNull()) {
        auto string = mSettings->getValue(DEFAULT_VIEW_ID).toString();
        if (string.isEmpty()) {
            string = "Icon View";
        } else {
            if (!mHash->contains(string))
                string = "Icon View";
        }
        mDefaultViewIdCache = string;
    }
    return mDefaultViewIdCache;
}

const QString DirectoryViewFactoryManager2::getDefaultViewId(int zoomLevel, const QString &uri)
{
    auto factorys = mHash->values();

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
        saveDefaultViewOption();
    }
}

void DirectoryViewFactoryManager2::saveDefaultViewOption()
{
    mSettings->setValue(DEFAULT_VIEW_ID, mDefaultViewIdCache);
}
