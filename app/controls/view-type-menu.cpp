#include "view-type-menu.h"

#include "view-factory-sort-filter-model.h"
#include "view/view-factory/directory-view-factory-manager.h"
#include "plugin-iface/directory-view-plugin-iface2.h"

#include <QActionGroup>

#include <QDebug>

ViewTypeMenu::ViewTypeMenu(QWidget *parent) : QMenu(parent)
{
    mModel = new ViewFactorySortFilterModel2(this);
    mViewActions = new QActionGroup(this);
    mViewActions->setExclusive(true);

    connect(mViewActions, &QActionGroup::triggered, this, [=](QAction *action) {
        auto viewId = action->data().toString();
        setCurrentView(viewId);
    });

    connect(this, &QMenu::aboutToShow, this, [=]() {
        qDebug()<<"show menu";
        updateMenuActions();
    });

    setCurrentDirectory("file:///");
}

void ViewTypeMenu::setCurrentView(const QString &viewId, bool blockSignal)
{
    if (viewId == mCurrentViewId)
        return;

    if (isViewIdValid(viewId)) {
        mCurrentViewId = viewId;
    }

    for (auto action : mViewActions->actions()) {
        if (action->text() == viewId) {
            action->setChecked(true);
        }
    }


    Q_EMIT this->switchViewRequest(viewId, mModel->iconFromViewId(viewId));

    if (!blockSignal) {
        auto factoryManager = DirectoryViewFactoryManager2::getInstance();
        auto factory = factoryManager->getFactory(viewId);
        int zoomLevelHint = factory->zoom_level_hint();
        Q_EMIT this->updateZoomLevelHintRequest(zoomLevelHint);
    }
}

void ViewTypeMenu::setCurrentDirectory(const QString &uri)
{
    mCurrentUri = uri;
    mModel->setDirectoryUri(uri);
}

bool ViewTypeMenu::isViewIdValid(const QString &viewId)
{
    return mModel->supportViewIds().contains(viewId);
}

void ViewTypeMenu::updateMenuActions()
{
    auto supportViews = mModel->supportViewIds();
    for (auto action : mViewActions->actions()) {
        removeAction(action);
        mViewActions->removeAction(action);
        action->deleteLater();
    }
    for (auto id : supportViews) {
        auto action = new QAction(this);
        auto text = mModel->getViewDisplayNameFromId(id);
        action->setText(text);
        action->setData(id);
        action->setIcon(mModel->iconFromViewId(id));
        mViewActions->addAction(action);
        addAction(action);

        action->setCheckable(true);
        if (mCurrentViewId == id) {
            action->setChecked(true);
        }
    }
}
