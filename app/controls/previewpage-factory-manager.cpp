#include "default-previewpage-factory.h"
#include "previewpage-factory-manager.h"

#include <QMap>

static PreviewPageFactoryManager *gInstance = nullptr;

PreviewPageFactoryManager *PreviewPageFactoryManager::getInstance()
{
    if (!gInstance) {
        gInstance = new PreviewPageFactoryManager;
    }
    return gInstance;
}

PreviewPageFactoryManager::PreviewPageFactoryManager(QObject *parent) : QObject(parent)
{
    mMap = new QMap<QString, PreviewPagePluginIface*>();
    //load default and plugins.
    auto defaultFactory = DefaultPreviewPageFactory::getInstance();
    registerFactory(defaultFactory->name(), static_cast<PreviewPagePluginIface*>(defaultFactory));
    //registerFactory("test", static_cast<PreviewPagePluginIface*>(defaultFactory));
}

PreviewPageFactoryManager::~PreviewPageFactoryManager()
{
    if (mMap) {
        //FIXME: unload all module?
        delete mMap;
    }
}

const QStringList PreviewPageFactoryManager::getPluginNames()
{
    QStringList l;
    for (auto key : mMap->keys()) {
        l << key;
    }
    return l;
}

bool PreviewPageFactoryManager::registerFactory(const QString &name, PreviewPagePluginIface *plugin)
{
    if (mMap->value(name)) {
        return false;
    }

    mMap->insert(name, plugin);
    return true;
}

PreviewPagePluginIface *PreviewPageFactoryManager::getPlugin(const QString &name)
{
    mLastPreviewPageId = name;
    return mMap->value(name);
}

const QString PreviewPageFactoryManager::getLastPreviewPageId()
{
    if (mLastPreviewPageId.isNull()) {
        return mMap->firstKey();
    }
    return mLastPreviewPageId;
}
