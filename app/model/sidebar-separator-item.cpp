#include "sidebar-separator-item.h"

#include "sidebar-model.h"

SideBarSeparatorItem::SideBarSeparatorItem(SideBarSeparatorItem::Details type, SideBarAbstractItem *parentItem, SideBarModel *model, QObject *parent) : SideBarAbstractItem(model, parent)
{
    mType = type;
    mParent = parentItem;
}

SideBarSeparatorItem::Details SideBarSeparatorItem::separatorType()
{
    return mType;
}

SideBarAbstractItem::Type SideBarSeparatorItem::type()
{
    return SideBarAbstractItem::SeparatorItem;
}

QString SideBarSeparatorItem::uri()
{
    return nullptr;
}

QString SideBarSeparatorItem::displayName()
{
    return mType==EmptyFile?tr("(No Sub Directory)"):nullptr;
}

QString SideBarSeparatorItem::iconName()
{
    return nullptr;
}

bool SideBarSeparatorItem::hasChildren()
{
    return false;
}

bool SideBarSeparatorItem::isRemoveable()
{
    return false;
}

bool SideBarSeparatorItem::isEjectable()
{
    return false;
}

bool SideBarSeparatorItem::isMountable()
{
    return false;
}

QModelIndex SideBarSeparatorItem::lastColumnIndex()
{
    return mModel->lastCloumnIndex(this);
}

QModelIndex SideBarSeparatorItem::firstColumnIndex()
{
    return mModel->firstCloumnIndex(this);
}

void SideBarSeparatorItem::eject()
{

}

void SideBarSeparatorItem::format()
{

}

void SideBarSeparatorItem::unmount()
{

}

void SideBarSeparatorItem::onUpdated()
{

}

void SideBarSeparatorItem::findChildren()
{

}

void SideBarSeparatorItem::clearChildren()
{

}

void SideBarSeparatorItem::findChildrenAsync()
{

}

SideBarAbstractItem *SideBarSeparatorItem::parent()
{
    return mParent;
}
