#ifndef SIDEBARPERSIONALITEM_H
#define SIDEBARPERSIONALITEM_H

#include "sidebar-abstract-item.h"



class SideBarPersonalItem : public SideBarAbstractItem
{
    Q_OBJECT
public:
    explicit SideBarPersonalItem(QString uri, SideBarPersonalItem *parentItem, SideBarModel *model, QObject *parent = nullptr);

    Type type () override;
    QString uri () override;
    bool isMountable() override;
    QString iconName () override;
    bool hasChildren () override;
    bool isEjectable () override;
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
    QString mUri = nullptr;
    bool mIsRootChild = false;
    QString mIconName = nullptr;
    QString mDisplayName = nullptr;
    SideBarPersonalItem *mParent = nullptr;
};

#endif // SIDEBARPERSIONALITEM_H
