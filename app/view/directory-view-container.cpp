#include "directory-view-container.h"

#include <QAction>
#include <file-utils.h>
#include <syslog/clib_syslog.h>
#include "directory-view-widget.h"
#include <model/fileitem-proxy-filter-sort-model.h>
#include <plugin-iface/directory-view-plugin-iface2.h>
#include <view-factory/directory-view-factory-manager.h>

DirectoryViewContainer::DirectoryViewContainer(QWidget *parent) : QWidget(parent)
{
    mModel = new FileItemModel(this);
    mProxyModel = new FileItemProxyFilterSortModel(this);
    mProxyModel->setSourceModel(mModel);

    setContentsMargins(0, 0, 0, 0);
    mLayout = new QVBoxLayout(this);
    mLayout->setMargin(0);
    mLayout->setContentsMargins(0, 0, 0, 0);
    mLayout->setSpacing(0);
    setLayout(mLayout);
}

DirectoryViewContainer::~DirectoryViewContainer()
{

}

bool DirectoryViewContainer::canCdUp()
{
    if (!mView)
        return false;
    return !FileUtils::getParentUri(mView->getDirectoryUri()).isNull();
}

bool DirectoryViewContainer::canGoBack()
{
    return !mBackList.isEmpty();
}

bool DirectoryViewContainer::canGoForward()
{
    return !mForwardList.isEmpty();
}

Qt::SortOrder DirectoryViewContainer::getSortOrder()
{
    if (!mView)
        return Qt::AscendingOrder;
    int order = mView->getSortOrder();
    return Qt::SortOrder(order);
}

const QString DirectoryViewContainer::getCurrentUri()
{
    if (mView) {
        return mView->getDirectoryUri();
    }

    CT_SYSLOG(LOG_WARNING, "return nullptr");
    return nullptr;
}

DirectoryViewWidget *DirectoryViewContainer::getView()
{
    return mView;
}

const QStringList DirectoryViewContainer::getBackList()
{
    QStringList l;
    for (auto uri : mBackList) {
        l << uri;
    }
    return l;
}

const QStringList DirectoryViewContainer::getAllFileUris()
{
    if (mView)
        return mView->getAllFileUris();
    return QStringList();
}

const QStringList DirectoryViewContainer::getForwardList()
{
    QStringList l;
    for (auto uri : mForwardList) {
        l << uri;
    }
    return l;
}

FileItemModel::ColumnType DirectoryViewContainer::getSortType()
{
    if (!mView)
        return FileItemModel::FileName;
    int type = mView->getSortType();
    return FileItemModel::ColumnType(type);
}

const QStringList DirectoryViewContainer::getCurrentSelections()
{
    if (mView)
        return mView->getSelections();
    return QStringList();
}

void DirectoryViewContainer::cdUp()
{
    if (!canCdUp())
        return;

    auto uri = FileUtils::getParentUri(mView->getDirectoryUri());
    if (uri.isNull())
        return;

    Q_EMIT updateWindowLocationRequest(uri, true);
}

void DirectoryViewContainer::goBack()
{
    if (!canGoBack())
        return;

    auto uri = mBackList.takeLast();
    mForwardList.prepend(getCurrentUri());
    Q_EMIT updateWindowLocationRequest(uri, false);
}

void DirectoryViewContainer::refresh()
{
    if (!mView)
        return;
    mView->beginLocationChange();
}

void DirectoryViewContainer::goForward()
{
    if (!canGoForward())
        return;

    auto uri = mForwardList.takeFirst();
    mBackList.append(getCurrentUri());

    Q_EMIT updateWindowLocationRequest(uri, false);
}

void DirectoryViewContainer::stopLoading()
{
    if (mView) {
        mView->stopLocationChange();
        Q_EMIT this->directoryChanged();
    }
}

void DirectoryViewContainer::updateFilter()
{
    mProxyModel->update();
}

void DirectoryViewContainer::clearHistory()
{
    mBackList.clear();
    mForwardList.clear();
}

void DirectoryViewContainer::clearConditions()
{
    mProxyModel->clearConditions();
}

void DirectoryViewContainer::tryJump(int index)
{
    QStringList l;
    l<<mBackList<<getCurrentUri()<<mForwardList;
    if (0 <= index && index < l.count()) {
        auto targetUri = l.at(index);
        mBackList.clear();
        mForwardList.clear();
        for (int i = 0; i < l.count(); i++) {
            if (i < index) {
                mBackList<<l.at(i);
            }
            if (i > index) {
                mForwardList<<l.at(i);
            }
        }
        Q_EMIT updateWindowLocationRequest(targetUri, false, true);
    }
}

void DirectoryViewContainer::setSortOrder(Qt::SortOrder order)
{
    if (order < 0)
        return;
    if (!mView)
        return;
    mView->setSortOrder(order);
}

void DirectoryViewContainer::setUseDefaultNameSortOrder(bool use)
{
    mProxyModel->setUseDefaultNameSortOrder(use);
}

void DirectoryViewContainer::setSortFolderFirst(bool folderFirst)
{
    mProxyModel->setFolderFirst(folderFirst);
}

void DirectoryViewContainer::switchViewType(const QString &viewId)
{
    if (getView()) {
        if (getView()->viewId() == viewId) {
            return;
        }
    }

    auto viewManager = DirectoryViewFactoryManager2::getInstance();
    auto factory = viewManager->getFactory(viewId);
    if (!factory)
        return;

    auto sortType = 0;
    auto sortOrder = 0;

    auto oldView = mView;
    QStringList selection;
    if (oldView) {
        sortType = oldView->getSortType();
        sortOrder = oldView->getSortOrder();
        selection = oldView->getSelections();
        mLayout->removeWidget(dynamic_cast<QWidget*>(oldView));
        oldView->deleteLater();
    }

    auto view = factory->create();
    mView = view;
    view->setParent(this);

    view->bindModel(mModel, mProxyModel);
//    view->setProxy(mProxy);

    view->setSortType(sortType);
    view->setSortOrder(sortOrder);

    connect(mView, &DirectoryViewWidget::menuRequest, this, &DirectoryViewContainer::menuRequest);
    connect(mView, &DirectoryViewWidget::viewDirectoryChanged, this, &DirectoryViewContainer::directoryChanged);
    connect(mView, &DirectoryViewWidget::viewDoubleClicked, this, &DirectoryViewContainer::viewDoubleClicked);
    connect(mView, &DirectoryViewWidget::viewDoubleClicked, this, &DirectoryViewContainer::onViewDoubleClicked);
    connect(mView, &DirectoryViewWidget::viewSelectionChanged, this, &DirectoryViewContainer::selectionChanged);

    connect(mView, &DirectoryViewWidget::zoomRequest, this, &DirectoryViewContainer::zoomRequest);

    //similar to double clicked, but just jump directory only.
    //note that if view use double clicked signal, this signal should
    //not sended again.
    connect(mView, &DirectoryViewWidget::updateWindowLocationRequest, this, [=](const QString &uri) {
        Q_EMIT this->updateWindowLocationRequest(uri, true);
    });

//    mProxy->switchView(view);
    mLayout->addWidget(dynamic_cast<QWidget*>(view), Qt::AlignBottom);
    DirectoryViewFactoryManager2::getInstance()->setDefaultViewId(viewId);
    if (!selection.isEmpty()) {
        view->setSelections(selection);
    }

    QAction *cdUpAction = new QAction(mView);
    cdUpAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::ALT + Qt::Key_Up));
    connect(cdUpAction, &QAction::triggered, this, [=]() {
        this->cdUp();
    });
    this->addAction(cdUpAction);

    QAction *goBackAction = new QAction(mView);
    goBackAction->setShortcut(QKeySequence::Back);
    connect(goBackAction, &QAction::triggered, this, [=]() {
        this->goBack();
    });
    this->addAction(goBackAction);

    QAction *goForwardAction = new QAction(mView);
    goForwardAction->setShortcut(QKeySequence::Forward);
    connect(goForwardAction, &QAction::triggered, this, [=]() {
        this->goForward();
    });
    this->addAction(goForwardAction);

    QAction *editAction = new QAction(mView);
    editAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::ALT + Qt::Key_E)<<Qt::Key_F2);
    connect(editAction, &QAction::triggered, this, [=]() {
        auto selections = mView->getSelections();
        if (selections.count() == 1) {
            mView->editUri(selections.first());
        }
    });
    this->addAction(editAction);

    Q_EMIT viewTypeChanged();
}

void DirectoryViewContainer::setShowHidden(bool showHidden)
{
    mProxyModel->setShowHidden(showHidden);
}

void DirectoryViewContainer::setFilterLabelConditions(QString name)
{
    mProxyModel->setFilterLabelConditions(name);
}

void DirectoryViewContainer::onViewDoubleClicked(const QString &uri)
{

}

void DirectoryViewContainer::setSortType(FileItemModel::ColumnType type)
{
    if (!mView)
        return;
    mView->setSortType(type);
}

void DirectoryViewContainer::addFilterCondition(int option, int classify, bool updateNow)
{
    mProxyModel->addFilterCondition(option, classify, updateNow);
}

void DirectoryViewContainer::goToUri(const QString &uri, bool addHistory, bool forceUpdate)
{
    int zoomLevel = -1;
    if (mView)
        zoomLevel = mView->currentZoomLevel();

    if (forceUpdate)
        goto update;

    if (uri.isNull())
        return;

    if (getCurrentUri() == uri)
        return;

update:
    if (addHistory) {
        mForwardList.clear();
        mBackList.append(getCurrentUri());
    }

    auto viewId = DirectoryViewFactoryManager2::getInstance()->getDefaultViewId(zoomLevel, uri);
    switchViewType(viewId);
    //update status bar zoom level
    updateStatusBarSliderStateRequest();
    if (zoomLevel < 0)
        zoomLevel = getView()->currentZoomLevel();

    setZoomLevelRequest(zoomLevel);
    //qDebug() << "setZoomLevelRequest:" <<zoomLevel;
    if (mView)
        mView->setCurrentZoomLevel(zoomLevel);

    mCurrentUri = uri;

    //special uri process
    if (mCurrentUri.endsWith("/."))
        mCurrentUri = mCurrentUri.left(mCurrentUri.length()-2);
    if (mCurrentUri.endsWith("/.."))
        mCurrentUri = mCurrentUri.left(mCurrentUri.length()-3);

    if (mView) {
        mView->setDirectoryUri(mCurrentUri);
        mView->beginLocationChange();
        //m_active_view_prxoy->setDirectoryUri(uri);
    }
}

void DirectoryViewContainer::removeFilterCondition(int option, int classify, bool updateNow)
{
    mProxyModel->removeFilterCondition(option, classify, updateNow);
}

void DirectoryViewContainer::setSortFilter(int FileTypeIndex, int FileMTimeIndex, int FileSizeIndex)
{
    mProxyModel->setFilterConditions(FileTypeIndex, FileMTimeIndex, FileSizeIndex);
}

void DirectoryViewContainer::bindNewProxy(DirectoryViewProxyIface *proxy)
{

}
