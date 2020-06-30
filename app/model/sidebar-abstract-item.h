#ifndef SIDEBARABSTRACTITEM_H
#define SIDEBARABSTRACTITEM_H

#include <QObject>

class SideBarAbstractItem : public QObject
{
    friend class SideBarModel;
    Q_OBJECT
public:
    enum Type
    {
        FavoriteItem,
        PersonalItem,
        FileSystemItem,
        SeparatorItem
    };

    explicit SideBarAbstractItem (SideBarModel* model, QObject *parent = nullptr);
    virtual ~SideBarAbstractItem ();

    virtual bool isMounted ();
    virtual Type type () = 0;
    virtual QString uri () = 0;
    virtual QString iconName () = 0;
    virtual bool hasChildren () = 0;
    virtual bool isEjectable () = 0;
    virtual bool isMountable () = 0;
    virtual bool isRemoveable () = 0;
    virtual QString displayName () = 0;

    virtual QModelIndex lastColumnIndex () = 0;
    virtual SideBarAbstractItem *parent () = 0;
    virtual QModelIndex firstColumnIndex () = 0;

Q_SIGNALS:
    void updated ();
    void findChildrenFinished ();

public Q_SLOTS:
    virtual void eject () = 0;
    virtual void format () = 0;
    virtual void unmount () = 0;
    virtual void onUpdated () = 0;
    virtual void clearChildren ();
    virtual void findChildren () = 0;
    virtual void findChildrenAsync () = 0;

protected:
    SideBarModel*                   mModel = nullptr;
    QVector<SideBarAbstractItem*>*  mChildren = nullptr;
};

#endif // SIDEBARABSTRACTITEM_H
