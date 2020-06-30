#include "sidebar-favorite-item.h"

#include <QAbstractItemModel>


SideBarFavoriteItem::SideBarFavoriteItem(QString uri, SideBarFavoriteItem *parentItem, SideBarModel *model, QObject *parent) : SideBarAbstractItem(model, parent)
{

}

SideBarAbstractItem::Type SideBarFavoriteItem::type()
{

}

QString SideBarFavoriteItem::uri()
{

}

QString SideBarFavoriteItem::displayName()
{

}

QString SideBarFavoriteItem::iconName()
{

}

bool SideBarFavoriteItem::hasChildren()
{

}

bool SideBarFavoriteItem::isRemoveable()
{
    return false;
}

bool SideBarFavoriteItem::isEjectable()
{
    return false;
}

bool SideBarFavoriteItem::isMountable()
{
    return false;
}

QModelIndex SideBarFavoriteItem::firstColumnIndex()
{

}

void SideBarFavoriteItem::eject()
{

}

void SideBarFavoriteItem::format()
{

}

void SideBarFavoriteItem::unmount()
{

}

void SideBarFavoriteItem::onUpdated()
{

}

void SideBarFavoriteItem::findChildren()
{

}

void SideBarFavoriteItem::clearChildren()
{

}

void SideBarFavoriteItem::findChildrenAsync()
{

}

QModelIndex SideBarFavoriteItem::lastColumnIndex()
{

}

SideBarAbstractItem *SideBarFavoriteItem::parent()
{
    return mParent;
}

void SideBarFavoriteItem::syncBookMark()
{

}
