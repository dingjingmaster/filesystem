#include "list-view-factory.h"
#include "list-view.h"

static ListViewFactory *globalInstance = nullptr;
static ListViewFactory2 *globalInstance2 = nullptr;

ListViewFactory *ListViewFactory::getInstance()
{
    if (!globalInstance) {
        globalInstance = new ListViewFactory;
    }
    return globalInstance;
}

const QString ListViewFactory::name()
{
    return QObject::tr("List View");
}

PluginIface::PluginType ListViewFactory::pluginType()
{
    return PluginType::DirectoryViewPlugin;
}

const QString ListViewFactory::description()
{
    return QObject::tr("Show the folder children as rows in a list.");
}

void ListViewFactory::setEnable(bool enable)
{
    Q_UNUSED(enable)
}

DirectoryViewIface *ListViewFactory::create()
{
    return new ListView;
}

QString ListViewFactory::viewIdentity()
{
    return QObject::tr("List View");
}

QIcon ListViewFactory::viewIcon()
{
    return QIcon::fromTheme("view-list-symbolic", QIcon::fromTheme("folder"));
}

bool ListViewFactory::supportUri(const QString &uri)
{
    return !uri.isEmpty();
}

ListViewFactory::ListViewFactory(QObject *parent) : QObject(parent)
{

}

ListViewFactory::~ListViewFactory()
{

}

int ListViewFactory::zoom_level_hint()
{
    return 75;
}

int ListViewFactory::priority(const QString &)
{
    return 0;
}

bool ListViewFactory::isEnable()
{
    return true;
}

const QIcon ListViewFactory::icon()
{
    return QIcon::fromTheme("view-list-symbolic", QIcon::fromTheme("folder"));
}

ListViewFactory2 *ListViewFactory2::getInstance()
{
    if (!globalInstance2) {
        globalInstance2 = new ListViewFactory2;
    }
    return globalInstance2;
}

const QString ListViewFactory2::name()
{
    return QObject::tr("List View");
}

PluginIface::PluginType ListViewFactory2::pluginType()
{
    return PluginType::DirectoryViewPlugin;
}

const QString ListViewFactory2::description()
{
    return QObject::tr("Show the folder children as rows in a list.");
}

const QIcon ListViewFactory2::icon()
{
    return QIcon::fromTheme("view-list-symbolic", QIcon::fromTheme("folder"));
}

void ListViewFactory2::setEnable(bool enable)
{
    Q_UNUSED(enable)
}

bool ListViewFactory2::supportUri(const QString &uri)
{
    return !uri.isEmpty();
}

ListViewFactory2::ListViewFactory2(QObject *parent) : QObject(parent)
{

}

ListViewFactory2::~ListViewFactory2()
{

}

bool ListViewFactory2::supportZoom()
{
    return true;
}

int ListViewFactory2::priority(const QString &)
{
    return 0;
}

int ListViewFactory2::maximumSupportedZoomLevel()
{
    return 20;
}

int ListViewFactory2::minimumSupportedZoomLevel()
{
    return 0;
}

int ListViewFactory2::zoom_level_hint()
{
    return 0;
}

DirectoryViewWidget *ListViewFactory2::create()
{
    return new ListView2;
}

QString ListViewFactory2::viewName()
{
    return name();
}

QIcon ListViewFactory2::viewIcon()
{
    return QIcon::fromTheme("view-list-symbolic", QIcon::fromTheme("folder"));
}

QString ListViewFactory2::viewIdentity()
{
    return "List View";
}

bool ListViewFactory2::isEnable()
{
    return true;
}
