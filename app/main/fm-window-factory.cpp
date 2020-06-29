#include "fm-window-factory.h"
#include "fm-window.h"

FMWindowFactory* FMWindowFactory::mGlobalInstance = nullptr;

FMWindowFactory *FMWindowFactory::getInstance()
{
    if (nullptr == mGlobalInstance) {
        mGlobalInstance = new FMWindowFactory();
    }

    return mGlobalInstance;
}

FMWindowIface *FMWindowFactory::create(const QStringList &uris)
{
    if (uris.empty()) {
        return nullptr;
    }

    auto window = new FMWindow (uris.first());
    QStringList l;
    for (auto uri : uris) {
        if (uris.indexOf(uri) != 0) {
            l << uri;
        }
    }
    if (!l.isEmpty()) {
//        window->addNewTabs (l);
    }

    return window;
}

FMWindowIface *FMWindowFactory::create(const QString &uri)
{
    return new FMWindow (uri);
}

FMWindowFactory::FMWindowFactory(QObject *parent) : QObject(parent)
{

}
