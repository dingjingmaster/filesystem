#include "navigation-tool-bar.h"

#include <QMenu>
#include <QToolButton>
#include "file-utils.h"
#include "directory-view-container.h"

NavigationToolBar::NavigationToolBar(QWidget *parent) : QToolBar(parent)
{
    mBackAction = addAction(QIcon::fromTheme("go-previous"), tr("Go Back"), [=]() {
        this->onGoBack();
    });

    mForwardAction = addAction(QIcon::fromTheme("go-next"), tr("Go Forward"), [=]() {
        this->onGoForward();
    });

    mHistoryAction = addAction(QIcon::fromTheme("pan-down-symbolic", QIcon::fromTheme("go-down")), tr("History"));

    auto historyButtonWidget = widgetForAction(mHistoryAction);
    auto historyButton = qobject_cast<QToolButton*>(historyButtonWidget);
    //historyButton->setToolButtonStyle(Qt::ToolButtonFollowStyle);
    //historyButton->setArrowType(Qt::NoArrow);
    //historyButton->setPopupMode(QToolButton::DelayedPopup);

    connect(mHistoryAction, &QAction::triggered, [=]() {
        QMenu historyMenu;
        //historyButton->setMenu(&historyMenu);
        auto back_list = mCurrentContainer->getBackList();
        auto current_uri = mCurrentContainer->getCurrentUri();
        auto forward_list = mCurrentContainer->getForwardList();
        QStringList historyList;
        historyList<<back_list;
        int currentIndex = historyList.count();
        historyList<<current_uri<<forward_list;
        QList<QAction*> actions;
        int count = 0;
        for (auto uri : historyList) {
            count++;
            auto action = historyMenu.addAction(QString::number(count) + ". " + uri);
            if (historyMenu.actions().indexOf(action) == currentIndex) {
                action->setCheckable(true);
                action->setChecked(true);
            }
            actions<<action;
        }
        historyMenu.addSeparator();
        historyMenu.addAction(QIcon::fromTheme("window-close-symbolic"), tr("Clear History"));
        //historyButton->showMenu();
        auto result = historyMenu.exec(historyButtonWidget->mapToGlobal(historyButton->rect().bottomLeft()));
        int clicked_index = historyMenu.actions().indexOf(result);
        if (clicked_index == historyMenu.actions().count() - 1) {
            //FIXME: clear history.
            mBackAction->setDisabled(true);
            mForwardAction->setDisabled(true);
            mCurrentContainer->clearHistory();
        }

        mCurrentContainer->tryJump(clicked_index);
        //historyButton->setMenu(nullptr);
    });

    mCdUpAction = addAction(QIcon::fromTheme("go-up"), tr("Cd Up"), [=]() {
        if (mCurrentContainer) {
            mCurrentContainer->cdUp();
        }
    });

    mRefreshAction = addAction(QIcon::fromTheme("gtk-refresh"), tr("Refresh"), [=]() {
        Q_EMIT refreshRequest();
    });

    updateActions();
}

void NavigationToolBar::updateActions()
{
    mBackAction->setEnabled(canGoBack());
    mForwardAction->setEnabled(canGoForward());
    mCdUpAction->setEnabled(canCdUp());
}

bool NavigationToolBar::canCdUp()
{
    if (!mCurrentContainer)
        return false;
    return mCurrentContainer->canCdUp();
}

bool NavigationToolBar::canGoBack()
{
    if (!mCurrentContainer)
        return false;
    return mCurrentContainer->canGoBack();
}

bool NavigationToolBar::canGoForward()
{
    if (!mCurrentContainer)
        return false;
    return mCurrentContainer->canGoForward();
}

void NavigationToolBar::onGoBack()
{
    if (canGoBack()) {
        mCurrentContainer->goBack();
    }
}

void NavigationToolBar::onGoForward()
{
    if (canGoForward()) {
        mCurrentContainer->goForward();
    }
}

void NavigationToolBar::onGoToUri(const QString &uri, bool addHistory, bool forceUpdate)
{
    if (mCurrentContainer) {
        mCurrentContainer->goToUri(uri, addHistory);
        updateWindowLocationRequest(mCurrentContainer->getCurrentUri(), false, forceUpdate);
    }
}

void NavigationToolBar::clearHistory()
{

}

void NavigationToolBar::setCurrentContainer(DirectoryViewContainer *container)
{
    if (mCurrentContainer == container)
        return;
    mCurrentContainer = container;
    Q_EMIT updateWindowLocationRequest(container->getCurrentUri(), false);
}
