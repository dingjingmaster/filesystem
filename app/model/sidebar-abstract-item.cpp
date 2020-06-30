#include "sidebar-abstract-item.h"

#include "sidebar-model.h"

SideBarAbstractItem::SideBarAbstractItem(SideBarModel *model, QObject *parent) : QObject(parent)
{
    mModel = model;
    mChildren = new QVector<SideBarAbstractItem*> ();
}

SideBarAbstractItem::~SideBarAbstractItem()
{
    for (auto child : *mChildren) {
        delete child;
    }
    delete mChildren;
}

bool SideBarAbstractItem::isMounted()
{
    return false;
}

void SideBarAbstractItem::clearChildren()
{
    mModel->removeRows(0, mChildren->count(), firstColumnIndex());
    for (auto child : *mChildren) {
        mChildren->removeOne(child);
        child->deleteLater();
    }
    mChildren->clear();
}
