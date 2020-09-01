#include "navigation-tool-bar.h"

#include <QPainter>
#include "navigation-bar.h"
#include "advanced-location-bar.h"
#include <directory-view-container.h>
#include "previewpage-factory-manager.h"

NavigationBar::NavigationBar(QWidget *parent) : QToolBar(parent)
{
    setContentsMargins(0, 0, 5, 0);
    setFixedHeight(38);
    setMovable(false);
    setFloatable(false);
    mLeftControl = new NavigationToolBar(this);
    mLeftControl->setFixedWidth(mLeftControl->sizeHint().width());

    mLeftControl->setContentsMargins(0, 0, 0, 0);
    addWidget(mLeftControl);
    addSeparator();

    mCenterControl = new AdvancedLocationBar(this);
    mCenterControl->setContentsMargins(0, 0, 0, 0);
    addWidget(mCenterControl);
    addSeparator();

    connect(mLeftControl, &NavigationToolBar::updateWindowLocationRequest,
            this, &NavigationBar::updateWindowLocationRequest);
    connect(mLeftControl, &NavigationToolBar::refreshRequest,
            this, &NavigationBar::refreshRequest);
    connect(mCenterControl, &AdvancedLocationBar::updateWindowLocationRequest,
            this, &NavigationBar::updateWindowLocationRequest);
    connect(mCenterControl, &AdvancedLocationBar::refreshRequest,
            this, &NavigationBar::refreshRequest);

    auto manager = PreviewPageFactoryManager::getInstance();
    auto ids = manager->getPluginNames();
    QActionGroup *group = new QActionGroup(this);
    mGroup = group;
    group->setExclusive(true);
    for (auto id : ids) {
        auto factory = manager->getPlugin(id);
        auto action = group->addAction(factory->icon(), factory->name());
        action->setCheckable(true);
        connect(action, &QAction::triggered, [=]() {
            if (mCheckedPreviewAction == action) {
                action->setChecked(false);
                mCheckedPreviewAction = nullptr;
            } else {
                mCheckedPreviewAction = action;
                action->setChecked(true);
            }
            mLastPreviewPageIdInWindow = id;
            Q_EMIT this->switchPreviewPageRequest(mCheckedPreviewAction? mCheckedPreviewAction->text(): nullptr);
        });
    }
    addActions(group->actions());
}

NavigationBar::~NavigationBar()
{

}

void NavigationBar::bindContainer(DirectoryViewContainer *container)
{
    mLeftControl->setCurrentContainer(container);
    mLeftControl->updateActions();
    updateLocation(container->getCurrentUri());
}

void NavigationBar::updateLocation(const QString &uri)
{
    mCenterControl->updateLocation(uri);
    mLeftControl->updateActions();
}

void NavigationBar::setBlock(bool block)
{
    this->blockSignals(block);
    mLeftControl->blockSignals(block);
    mCenterControl->blockSignals(block);
    mLeftControl->setDisabled(block);
    mCenterControl->setDisabled(block);
}

void NavigationBar::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    auto color = this->palette().base().color();
    color.setAlpha(127);
    p.fillRect(this->rect().adjusted(-1, -1, 1, 1), color);
    QToolBar::paintEvent(e);
}

bool NavigationBar::isPathEditing()
{
    return mCenterControl->isEditing();
}

const QString NavigationBar::getLastPreviewPageId()
{
    if (mLastPreviewPageIdInWindow.isNull()) {
        return PreviewPageFactoryManager::getInstance()->getLastPreviewPageId();
    }
    return mLastPreviewPageIdInWindow;
}

void NavigationBar::startEdit()
{
    mCenterControl->startEdit();
}

void NavigationBar::finishEdit()
{
    mCenterControl->finishEdit();
}

void NavigationBar::triggerAction(const QString &id) {
    for (auto action : mGroup->actions()) {
        if (action->text() == id) {
            action->trigger();
            return;
        }
    }
}
