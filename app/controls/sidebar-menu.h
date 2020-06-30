#ifndef SIDEBARMENU_H
#define SIDEBARMENU_H

#include <QMenu>

class SideBar;
class SideBarAbstractItem;

class SideBarMenu : public QMenu
{
    Q_OBJECT
public:
    explicit SideBarMenu (SideBarAbstractItem *item, SideBar *sideBar, QWidget *parent = nullptr);

protected:
    const QList<QAction*> constructFavoriteActions ();
    const QList<QAction*> constructPersonalActions ();
    const QList<QAction*> constructFileSystemItemActions ();

private:
    QString mUri;
    SideBar *mSideBar;
    SideBarAbstractItem *mItem;
};

#endif // SIDEBARMENU_H
