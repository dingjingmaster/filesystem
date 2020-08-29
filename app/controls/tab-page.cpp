#include "tab-page.h"

#include "view/view-factory/directory-view-factory-manager.h"

#include <QTabBar>
#include <file-utils.h>
#include <file/file-info.h>
#include <directory-view-widget.h>
#include <file/file-launch-manager.h>
#include <directory-view-container.h>
#include <window/properties-window.h>


TabPage::TabPage(QWidget *parent) : QTabWidget(parent)
{
    mDoubleClickLimiter.setSingleShot(true);

    setMovable(true);
    setDocumentMode(true);
    setElideMode(Qt::ElideRight);
    setTabsClosable(true);
    setUsesScrollButtons(true);
    tabBar()->setExpanding(false);
    tabBar()->setAutoHide(true);

    connect(this, &QTabWidget::tabCloseRequested, [=](int index) {
        auto container = dynamic_cast<DirectoryViewContainer*>(widget(index));
        container->disconnect();
        container->deleteLater();
    });

    connect(this, &QTabWidget::currentChanged, [=](int index) {
        Q_UNUSED(index)
        Q_EMIT this->currentActiveViewChanged();

        this->rebindContainer();
    });
}

DirectoryViewContainer *TabPage::getActivePage()
{
    return qobject_cast<DirectoryViewContainer*>(currentWidget());
}

void TabPage::addPage(const QString &uri)
{
    auto container = new DirectoryViewContainer(this);
    auto view = container->getView();
    container->switchViewType(DirectoryViewFactoryManager2::getInstance()->getDefaultViewId());
    container->getView()->setDirectoryUri(uri);
    container->getView()->beginLocationChange();
    addTab(container, QIcon::fromTheme(FileUtils::getFileIconName(uri), QIcon::fromTheme("folder")), FileUtils::getFileDisplayName(uri));
    rebindContainer();
}

void TabPage::rebindContainer()
{
    for (int i = 0; i < this->count(); i++) {
        this->widget(i)->disconnect();
    }

    auto container = getActivePage();
    container->connect(container, &DirectoryViewContainer::viewDoubleClicked, [=](const QString &uri) {
        if (mDoubleClickLimiter.isActive())
            return;

        mDoubleClickLimiter.start(500);

        auto info = FileInfo::fromUri(uri, false);
        if (info->uri().startsWith("trash://")) {
            auto w = new PropertiesWindow(QStringList()<<uri);
            w->show();
            return;
        }
        if (info->isDir() || info->isVolume() || info->isVirtual()) {
            Q_EMIT this->updateWindowLocationRequest(uri);
        } else {
            FileLaunchManager::openAsync(uri, false, false);
        }
    });

    container->connect(container, &DirectoryViewContainer::updateWindowLocationRequest, this, &TabPage::updateWindowLocationRequest);
    container->connect(container, &DirectoryViewContainer::directoryChanged, this, &TabPage::currentLocationChanged);
    container->connect(container, &DirectoryViewContainer::selectionChanged, this, &TabPage::currentSelectionChanged);
    container->connect(container, &DirectoryViewContainer::menuRequest, this, &TabPage::menuRequest);
    container->connect(container, &DirectoryViewContainer::viewTypeChanged, this, &TabPage::viewTypeChanged);
}

void TabPage::refreshCurrentTabText()
{
    auto uri = getActivePage()->getCurrentUri();
    setTabText(currentIndex(), FileUtils::getFileDisplayName(uri));
    setTabIcon(currentIndex(), QIcon::fromTheme(FileUtils::getFileIconName(uri), QIcon::fromTheme("folder")));
}

void TabPage::stopLocationChange()
{
    auto view = getActivePage();
    view->stopLoading();
}
