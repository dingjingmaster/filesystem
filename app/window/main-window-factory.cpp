#include "main-window-factory.h"
#include "main-window.h"

MainWindowFactory *gInstance = nullptr;

FMWindowFactory *MainWindowFactory::getInstance()
{
    auto g = gInstance;
    if (!gInstance) {
        gInstance = new MainWindowFactory;
    }

    return gInstance;
}

FMWindowIface *MainWindowFactory::create(const QString &uri)
{
    auto window = new MainWindow(uri);
    return window;
}

FMWindowIface *MainWindowFactory::create(const QStringList &uris)
{
    if (uris.isEmpty())
        return new MainWindow;
    auto uri = uris.first();
    auto l = uris;
    l.removeAt(0);
    auto window = new MainWindow(uri);
    window->slotAddNewTabs(l);
    return window;
}

MainWindowFactory::MainWindowFactory(QObject *parent) : FMWindowFactory(parent)
{

}
