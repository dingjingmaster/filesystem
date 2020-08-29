#include "sidebar-menu.h"

#include "menu-plugin-manager.h"
#include "model/sidebar-abstract-item.h"

#include <window/bookmark-manager.h>
#include <window/properties-window.h>

#include <file/file-info.h>
#include <file/file-info-job.h>

#include <QAction>
#include <QModelIndex>

SideBarMenu::SideBarMenu(SideBarAbstractItem *item, SideBar *sideBar, QWidget *parent) : QMenu (parent)
{
    mUri = item->uri();
    mItem = item;
    mSideBar = sideBar;

    if (!item) {
        auto action = addAction(QIcon::fromTheme("preview-file"), tr("&Properties"));
        action->setEnabled(false);
        return;
    }

    switch (item->type()) {
    case SideBarAbstractItem::FavoriteItem: {
        constructFavoriteActions();
        break;
    }
    case SideBarAbstractItem::PersonalItem: {
        constructPersonalActions();
        break;
    }
    case SideBarAbstractItem::FileSystemItem: {
        constructFileSystemItemActions();
        break;
    }
    default: {
        auto action = addAction(QIcon::fromTheme("preview-file"), tr("&Properties"));
        action->setEnabled(false);
        break;
    }
    }
}

const QList<QAction *> SideBarMenu::constructFavoriteActions()
{
    QList<QAction*> l;
    l<<addAction(QIcon::fromTheme("window-close-symbolic"), tr("&Delete Symbolic"), [=]() {
        BookMarkManager::getInstance()->removeBookMark(mUri);
    });
    if (!mItem->firstColumnIndex().parent().isValid()) {
        l.last()->setEnabled(false);
    } else if (mItem->firstColumnIndex().row() < 3) {
        l.last()->setEnabled(false);
    }

    l << addAction(QIcon::fromTheme("preview-file"), tr("&Properties"), [=]() {
        PropertiesWindow *w = new PropertiesWindow(QStringList() << mUri);
        w->show();
    });
    if (!mItem->firstColumnIndex().parent().isValid()) {
        l.last()->setEnabled(false);
    }

    return l;
}

const QList<QAction *> SideBarMenu::constructPersonalActions()
{
    QList<QAction*> l;

    l<<addAction(QIcon::fromTheme("preview-file"), tr("&Properties"), [=]() {
        PropertiesWindow *w = new PropertiesWindow(QStringList() << mUri);
        w->show();
    });

    return l;
}

const QList<QAction *> SideBarMenu::constructFileSystemItemActions()
{
    QList<QAction *> l;

    auto info = FileInfo::fromUri(mUri);
    if (info->displayName().isEmpty()) {
        FileInfoJob j(info);
        j.querySync();
    }
    if (info->canUnmount() || info->canMount()) {
        l<<addAction(QIcon::fromTheme("media-eject"), tr("&Unmount"), [=]() {
            mItem->unmount();
        });
        l.last()->setEnabled(mItem->isMounted());
    }

    auto mgr = MenuPluginManager::getInstance();
    auto ids = mgr->getPluginIds();
    for (auto id : ids) {
        auto factory = mgr->getPlugin(id);
        auto tmp = factory->menuActions(MenuPluginIface::SideBar, mUri, QStringList() << mUri);
        addActions(tmp);
        for (auto action : tmp) {
            action->setParent(this);
        }
        l << tmp;
    }

    l<<addAction(QIcon::fromTheme("preview-file"), tr("&Properties"), [=]() {
        PropertiesWindow *w = new PropertiesWindow(QStringList()<<mUri);
        w->show();
    });

    return l;
}
