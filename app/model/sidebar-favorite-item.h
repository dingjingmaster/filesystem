#ifndef SIDEBARFAVORITEITEM_H
#define SIDEBARFAVORITEITEM_H

#include "sidebar-abstract-item.h"



class SideBarFavoriteItem : public SideBarAbstractItem
{
    Q_OBJECT
public:
    explicit SideBarFavoriteItem(QString uri, SideBarFavoriteItem *parentItem, SideBarModel *model, QObject *parent = nullptr);

    Type type () override;
    QString uri () override;
    QString iconName () override;
    bool hasChildren () override;
    bool isEjectable () override;
    bool isMountable () override;
    bool isRemoveable () override;
    QString displayName () override;
    QModelIndex lastColumnIndex () override;
    SideBarAbstractItem *parent () override;
    QModelIndex firstColumnIndex () override;

public Q_SLOTS:
    void eject () override;
    void format () override;
    void unmount () override;
    void onUpdated () override;
    void findChildren () override;
    void clearChildren () override;
    void findChildrenAsync () override;

private:
    void syncBookMark ();

    QString mUri = nullptr;
    bool mIsRootChild = false;
    QString mIconName = nullptr;
    QString mDisplayName = nullptr;
    SideBarFavoriteItem *mParent = nullptr;
};

#endif // SIDEBARFAVORITEITEM_H
