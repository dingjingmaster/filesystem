#ifndef SIDEBARSEPARATORITEM_H
#define SIDEBARSEPARATORITEM_H

#include "sidebar-abstract-item.h"



class SideBarSeparatorItem : public SideBarAbstractItem
{
    Q_OBJECT
public:
    enum Details
    {
        Large,
        EmptyFile,
        Small
    };

    explicit SideBarSeparatorItem (Details type, SideBarAbstractItem *parentItem, SideBarModel *model, QObject *parent = nullptr);

    Details separatorType ();

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
    Details mType;
    SideBarAbstractItem *mParent = nullptr;
};

#endif // SIDEBARSEPARATORITEM_H
