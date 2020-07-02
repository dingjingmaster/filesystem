#include "icon-view-factory.h"


IconViewFactory2 *IconViewFactory2::getInstance()
{

}

const QString IconViewFactory2::name()
{
    return QObject::tr("Icon View");
}

PluginIface::PluginType IconViewFactory2::pluginType()
{
    return PluginType::DirectoryViewPlugin;
}

const QString IconViewFactory2::description()
{
    return QObject::tr("Show the folder children as icons.");
}

const QIcon IconViewFactory2::icon()
{
    return QIcon::fromTheme("view-grid-symbolic", QIcon::fromTheme("folder"));
}

void IconViewFactory2::setEnable(bool enable)
{
    Q_UNUSED(enable)
}

bool IconViewFactory2::isEnable()
{
    return true;
}

QString IconViewFactory2::viewIdentity()
{
    return "Icon View";
}

QString IconViewFactory2::viewName()
{
    return name();
}

QIcon IconViewFactory2::viewIcon()
{
    return QIcon::fromTheme("view-grid-symbolic", QIcon::fromTheme("folder"));
}

bool IconViewFactory2::supportUri(const QString &uri)
{
    return !uri.isEmpty();
}

IconViewFactory2::IconViewFactory2(QObject *parent) : QObject(parent)
{

}

IconViewFactory2::~IconViewFactory2()
{

}

DirectoryViewWidget *IconViewFactory2::create()
{

}

int IconViewFactory2::zoom_level_hint()
{
    return 25;
}

int IconViewFactory2::minimumSupportedZoomLevel()
{
    return 21;
}

int IconViewFactory2::maximumSupportedZoomLevel()
{
    return 100;
}

int IconViewFactory2::priority(const QString &)
{
    return 0;
}

bool IconViewFactory2::supportZoom()
{
    return true;
}

IconViewFactory *IconViewFactory::getInstance()
{

}

const QString IconViewFactory::name()
{
    return QObject::tr("Icon View");
}

PluginIface::PluginType IconViewFactory::pluginType()
{
    return PluginType::DirectoryViewPlugin;
}

const QString IconViewFactory::description()
{
    return QObject::tr("Show the folder children as icons.");
}

const QIcon IconViewFactory::icon()
{
    return QIcon::fromTheme("view-grid-symbolic", QIcon::fromTheme("folder"));
}

void IconViewFactory::setEnable(bool enable)
{
    Q_UNUSED(enable)
}

bool IconViewFactory::supportUri(const QString &uri)
{
    return !uri.isEmpty();
}

IconViewFactory::IconViewFactory(QObject *parent) : QObject (parent)
{

}

IconViewFactory::~IconViewFactory()
{

}

int IconViewFactory::zoom_level_hint()
{
    return 100;
}

int IconViewFactory::priority(const QString &)
{
    return 0;
}

DirectoryViewIface *IconViewFactory::create()
{

}

QIcon IconViewFactory::viewIcon()
{
    return QIcon::fromTheme("view-grid-symbolic", QIcon::fromTheme("folder"));
}

QString IconViewFactory::viewIdentity()
{
    return QObject::tr("Icon View");
}

bool IconViewFactory::isEnable()
{
    return true;
}

