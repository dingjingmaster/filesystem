#include "directory-view-factory-manager2.h"
#include "previewpage-factory-manager.h"
#include "tab-statusbar.h"
#include "tabwidget.h"

#include <QTimer>
#include <QToolBar>
#include <QSplitter>
#include <QToolButton>
#include <QFileDialog>
#include <file-utils.h>
#include <QActionGroup>
#include <QApplication>
#include <QStandardPaths>
#include <QStringListModel>
#include <global-settings.h>
#include <syslog/clib_syslog.h>
#include <directory-view-widget.h>
#include <file/file-launch-manager.h>
#include <window/properties-window.h>
#include <vfs/search-vfs-uri-parser.h>

TabWidget::TabWidget(QWidget *parent) : QMainWindow(parent)
{
//    setStyle(MainWindowStyle::getStyle());

    setAttribute(Qt::WA_TranslucentBackground);

    mTabBar = new NavigationTabBar(this);
    mTabBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mStack = new QStackedWidget(this);
    mStack->setContentsMargins(0, 0, 0, 0);
    mButtons = new PreviewPageButtonGroups(this);
    mPreviewPageContainer = new QStackedWidget(this);
    mPreviewPageContainer->setMinimumWidth(200);

    //status bar
    mStatusBar = new TabStatusBar(this, this);
    connect(this, &TabWidget::zoomRequest, mStatusBar, &TabStatusBar::onZoomRequest);
    connect(mStatusBar, &TabStatusBar::zoomLevelChangedRequest, this, &TabWidget::handleZoomLevel);
    setStatusBar(mStatusBar);

    connect(mButtons, &PreviewPageButtonGroups::previewPageButtonTrigger, [=](bool trigger, const QString &id) {
        setTriggeredPreviewPage(trigger);
        if (trigger) {
            auto plugin = PreviewPageFactoryManager::getInstance()->getPlugin(id);
            setPreviewPage(plugin->createPreviewPage());
        } else {
            setPreviewPage(nullptr);
        }
    });

    connect(mTabBar, &QTabBar::tabMoved, this, &TabWidget::moveTab);
    connect(mTabBar, &QTabBar::tabCloseRequested, this, &TabWidget::removeTab);
    connect(mTabBar, &NavigationTabBar::addPageRequest, this, &TabWidget::addPage);
    connect(mTabBar, &QTabBar::currentChanged, this, &TabWidget::changeCurrentIndex);
    connect(mTabBar, &NavigationTabBar::closeWindowRequest, this, &TabWidget::closeWindowRequest);
    connect(mTabBar, &NavigationTabBar::locationUpdated, this, &TabWidget::updateSearchPathButton);

    QHBoxLayout *t = new QHBoxLayout(this);
    QActionGroup *group = new QActionGroup(this);
    group->setExclusive(true);
    mTabBarBg = new QWidget(this);
    mTabBarBg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QToolBar *previewButtons = new QToolBar(this);
    previewButtons->setFixedHeight(mTabBar->height());
    t->setContentsMargins(0, 0, 5, 0);
    t->addWidget(mTabBarBg);

    updateTabBarGeometry();

    auto manager = PreviewPageFactoryManager::getInstance();
    auto pluginNames = manager->getPluginNames();
    for (auto name : pluginNames) {
        auto factory = manager->getPlugin(name);
        auto action = group->addAction(factory->icon(), factory->name());
        action->setCheckable(true);
        connect(action, &QAction::triggered, [=](/*bool checked*/) {
            if (!mCurrentPreviewAction) {
                mCurrentPreviewAction = action;
                action->setChecked(true);
                Q_EMIT mButtons->previewPageButtonTrigger(true, factory->name());
            } else {
                if (mCurrentPreviewAction == action) {
                    mCurrentPreviewAction = nullptr;
                    action->setChecked(false);
                    Q_EMIT mButtons->previewPageButtonTrigger(false, factory->name());
                } else {
                    mCurrentPreviewAction = action;
                    action->setChecked(true);
                    Q_EMIT mButtons->previewPageButtonTrigger(true, factory->name());
                }
            }
        });
    }

    previewButtons->addActions(group->actions());
    for (auto action : group->actions()) {
        auto button = qobject_cast<QToolButton *>(previewButtons->widgetForAction(action));
        button->setFixedSize(26, 26);
        button->setIconSize(QSize(16, 16));
        button->setProperty("useIconHighlightEffect", true);
        button->setProperty("iconHighlightEffectMode", 1);
        button->setProperty("fillIconSymbolicColor", true);
    }
    t->addWidget(previewButtons);

    //trash quick operate buttons
    QHBoxLayout *trash = new QHBoxLayout(this);
    mTrashBarLayout = trash;
    QToolBar *trashButtons = new QToolBar(this);
    mTrashBar = trashButtons;

    QLabel *Label = new QLabel(tr("Trash"), trashButtons);
    Label->setFixedHeight(TRASH_BUTTON_HEIGHT);
    Label->setFixedWidth(TRASH_BUTTON_WIDTH);
    mTrashLabel = Label;
    QPushButton *clearAll = new QPushButton(tr("Clear"), trashButtons);
    clearAll->setFixedWidth(TRASH_BUTTON_WIDTH);
    clearAll->setFixedHeight(TRASH_BUTTON_HEIGHT);
    mClearButton = clearAll;
    QPushButton *recover = new QPushButton(tr("Recover"), trashButtons);
    recover->setFixedWidth(TRASH_BUTTON_WIDTH);
    recover->setFixedHeight(TRASH_BUTTON_HEIGHT);
    mRecoverButton = recover;
    trash->addSpacing(10);
    trash->addWidget(Label, Qt::AlignLeft);
    trash->setContentsMargins(10, 0, 10, 0);
    trash->addWidget(trashButtons);
    trash->addWidget(recover, Qt::AlignLeft);
    trash->addSpacing(10);
    trash->addWidget(clearAll, Qt::AlignLeft);
    updateTrashBarVisible();

    connect(clearAll, &QPushButton::clicked, this, [=]() {
        Q_EMIT this->clearTrash();
    });

    connect(recover, &QPushButton::clicked, this, [=]() {
        Q_EMIT this->recoverFromTrash();
    });

    //advance search ui init
    initAdvanceSearch();

    QWidget *w = new QWidget();
    w->setAttribute(Qt::WA_TranslucentBackground);
    auto vbox = new QVBoxLayout();
    mTopLayout = vbox;
    vbox->setSpacing(0);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->addLayout(t);
    vbox->addLayout(trash);
    vbox->addLayout(mSearchBarLayout);
    QSplitter *s = new QSplitter(this);
    s->setChildrenCollapsible(false);
    s->setContentsMargins(0, 0, 0, 0);
    s->setHandleWidth(1);
    s->setStretchFactor(0, 1);
    s->addWidget(mStack);
    mStack->installEventFilter(this);
    s->addWidget(mPreviewPageContainer);
    mPreviewPageContainer->hide();
    vbox->addWidget(s);
    w->setLayout(vbox);
    setCentralWidget(w);

    //bind preview page
    connect(this, &TabWidget::activePageSelectionChanged, this, [=]() {
        updatePreviewPage();
        mStatusBar->update();
        Q_EMIT this->currentSelectionChanged();
    });

    connect(this, &TabWidget::activePageChanged, this, [=]() {
        QTimer::singleShot(100, this, [=]() {
            mStatusBar->update();
            this->updatePreviewPage();
        });
    });

    connect(this, &TabWidget::activePageLocationChanged, mStatusBar, [=]() {
        mStatusBar->update();
    });
}

void TabWidget::initAdvanceSearch()
{
    //advance search bar
    QHBoxLayout *search = new QHBoxLayout(this);
    mSearchBarLayout = search;
    QToolBar *searchButtons = new QToolBar(this);
    mSearchBar = searchButtons;
    QPushButton *closeButton = new QPushButton(QIcon::fromTheme("tab-close"), "", searchButtons);
    mSearchClose = closeButton;
    closeButton->setFixedHeight(20);
    closeButton->setFixedWidth(20);
    closeButton->setToolTip(tr("Close advance search."));

    connect(closeButton, &QPushButton::clicked, [=]() {
        updateSearchBar(false);
    });

    QLabel *title = new QLabel(tr("Search"), searchButtons);
    mSearchTitle = title;
    title->setFixedWidth(TRASH_BUTTON_WIDTH);
    title->setFixedHeight(TRASH_BUTTON_HEIGHT);

    QPushButton *tabButton = new QPushButton(searchButtons);
    mSearchPath = tabButton;
    tabButton->setFixedHeight(TRASH_BUTTON_HEIGHT);
    tabButton->setFixedWidth(TRASH_BUTTON_WIDTH * 2);
    tabButton->setToolTip(tr("Choose other path to search."));
    connect(tabButton, &QPushButton::clicked, this, &TabWidget::browsePath);

    QPushButton *childButton = new QPushButton(searchButtons);
    mSearchChild = childButton;
    childButton->setFixedHeight(TRASH_BUTTON_HEIGHT);
    childButton->setFixedWidth(TRASH_BUTTON_HEIGHT);
    childButton->setIcon(QIcon(":/custom/icons/child-folder"));
    childButton->setToolTip(tr("Search recursively"));
    connect(childButton, &QPushButton::clicked, this, &TabWidget::searchChildUpdate);
    searchChildUpdate();

    QPushButton *moreButton = new QPushButton(tr("more options"),searchButtons);
    mSearchMore = moreButton;
    moreButton->setFixedHeight(TRASH_BUTTON_HEIGHT);
    moreButton->setFixedWidth(TRASH_BUTTON_WIDTH *2);
    moreButton->setToolTip(tr("Show/hide advance search"));

    connect(moreButton, &QPushButton::clicked, this, &TabWidget::updateSearchList);

    search->addWidget(closeButton, Qt::AlignLeft);
    search->addSpacing(10);
    search->addWidget(title, Qt::AlignLeft);
    search->addSpacing(10);
    search->addWidget(tabButton, Qt::AlignLeft);
    search->addSpacing(10);
    search->addWidget(childButton, Qt::AlignLeft);
    search->addSpacing(10);
    search->addWidget(moreButton, Qt::AlignLeft);
    search->addSpacing(10);
    search->addWidget(searchButtons);
    search->setContentsMargins(10, 0, 10, 0);
    searchButtons->setVisible(false);
    tabButton->setVisible(false);
    closeButton->setVisible(false);
    title->setVisible(false);
    childButton->setVisible(false);
    moreButton->setVisible(false);
}

//search conditions changed, update filter
void TabWidget::searchUpdate()
{
    QString keyList = "";
    for(int i=0; i<mLayoutList.count(); i++) {
        if (mConditionsList[i]->currentIndex() == 0 && mInputList[i]->text() != "") {
            if (keyList == "") {
                keyList = mInputList[i]->text();
            } else {
                keyList += "," + mInputList[i]->text();
            }
        }
    }

    if (keyList != "") {
        if (mLastNonSearchPath == "") {
            mLastNonSearchPath = getCurrentUri();
        }
        auto targetUri = SearchVFSUriParser::parseSearchKey(mLastNonSearchPath, "", true, false, keyList, mSearchChildFlag);
        Q_EMIT this->updateWindowLocationRequest(targetUri, true);
    }
}

void TabWidget::searchKeyUpdate()
{
    searchUpdate();
}

void TabWidget::searchChildUpdate()
{
    mSearchChildFlag = ! mSearchChildFlag;
    mSearchChild->setCheckable(mSearchChildFlag);
    mSearchChild->setChecked(mSearchChildFlag);
    mSearchChild->setDown(mSearchChildFlag);
    searchUpdate();
}

void TabWidget::browsePath()
{
    QString targetPath = QFileDialog::getExistingDirectory(this, "caption", getCurrentUri(), QFileDialog::ShowDirsOnly);
    CT_SYSLOG(LOG_DEBUG, "browse path opened: %s", targetPath.toUtf8().constData());

    if (!targetPath.contains("file://") && targetPath != "") {
        targetPath = "file://" + targetPath;
    }

    if (targetPath != "" && targetPath != getCurrentUri()) {
        updateSearchPathButton(targetPath);
        Q_EMIT this->updateWindowLocationRequest(targetPath, true);
    }
}

void TabWidget::addNewConditionBar()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    mLayoutList.append(layout);

    QToolBar *optionBar = new QToolBar(this);
    mSearchBarList.append(optionBar);

    QComboBox *conditionCombox = new QComboBox(optionBar);
    mConditionsList.append(conditionCombox);
    conditionCombox->setFixedHeight(TRASH_BUTTON_HEIGHT);
    conditionCombox->setFixedWidth(TRASH_BUTTON_WIDTH *2);
    auto conditionModel = new QStringListModel(optionBar);
    conditionModel->setStringList(mOptionList);
    conditionCombox->setModel(conditionModel);
    auto index = mSearchBarCount;
    if (index > mOptionList.count()-1) {
        index = mOptionList.count()-1;
    }
    conditionCombox->setCurrentIndex(index);

    QLabel *linkLabel = new QLabel(tr("is"));
    mLinkLabelList.append(linkLabel);
    linkLabel->setFixedHeight(TRASH_BUTTON_HEIGHT);
    linkLabel->setFixedWidth(TRASH_BUTTON_HEIGHT);

    QComboBox *classifyCombox = new QComboBox(optionBar);
    mClassifyList.append(classifyCombox);
    classifyCombox->setFixedHeight(TRASH_BUTTON_HEIGHT);
    classifyCombox->setFixedWidth(TRASH_BUTTON_WIDTH *2);
    auto classifyModel = new QStringListModel(optionBar);
    auto list = getCurrentClassify(index);
    classifyModel->setStringList(list);
    classifyCombox->setModel(classifyModel);

    QLineEdit *inputBox = new QLineEdit(optionBar);
    mInputList.append(inputBox);
    inputBox->setFixedHeight(TRASH_BUTTON_HEIGHT);
    inputBox->setFixedWidth(TRASH_BUTTON_WIDTH *4);
    inputBox->setPlaceholderText(tr("Please input key words..."));

    QPushButton *addButton = new QPushButton(QIcon::fromTheme("add"), "", optionBar);
    mAddButtonList.append(addButton);
    addButton->setFixedHeight(20);
    addButton->setFixedWidth(20);
    connect(addButton, &QPushButton::clicked, this, &TabWidget::addNewConditionBar);

    QPushButton *removeButton = new QPushButton(QIcon::fromTheme("remove"), "", optionBar);
    mRemoveButtonList.append(removeButton);
    removeButton->setFixedHeight(20);
    removeButton->setFixedWidth(20);
    //mapper for button clicked parse index
    auto signalMapper = new QSignalMapper(this);
    connect(removeButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    signalMapper->setMapping(removeButton, index);
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(removeConditionBar(int)));
    mRemoveMapperList.append(signalMapper);

    layout->addSpacing(TRASH_BUTTON_WIDTH + 40);
    layout->addWidget(conditionCombox, Qt::AlignLeft);
    layout->addSpacing(10);
    layout->addWidget(linkLabel, Qt::AlignLeft);
    layout->addSpacing(10);
    layout->addWidget(classifyCombox, Qt::AlignLeft);
    layout->addWidget(inputBox, Qt::AlignLeft);
    layout->addWidget(optionBar);
    layout->addWidget(addButton, Qt::AlignRight);
    layout->addSpacing(10);
    layout->addWidget(removeButton, Qt::AlignRight);
    layout->setContentsMargins(10, 0, 10, 5);

    if (index == 0) {
        classifyCombox->hide();
        linkLabel->setFixedWidth(TRASH_BUTTON_WIDTH);
        linkLabel->setText(tr("contains"));
    } else {
        inputBox->hide();
    }

    connect(conditionCombox, &QComboBox::currentTextChanged, [=]() {
        auto cur = conditionCombox->currentIndex();
        if (cur == 0) {
            classifyCombox->hide();
            inputBox->show();
            linkLabel->setFixedWidth(TRASH_BUTTON_WIDTH);
            linkLabel->setText(tr("contains"));
        } else {
            classifyCombox->show();
            inputBox->hide();
            linkLabel->setFixedWidth(TRASH_BUTTON_HEIGHT);
            linkLabel->setText(tr("is"));
            auto classifyList = getCurrentClassify(cur);
            classifyModel->setStringList(classifyList);
            classifyCombox->setModel(classifyModel);
            classifyCombox->setCurrentIndex(0);
        }
    });

    connect(classifyCombox, &QComboBox::currentTextChanged, this, &TabWidget::updateAdvanceConditions);
    connect(inputBox, &QLineEdit::textChanged, this, &TabWidget::searchKeyUpdate);
    connect(inputBox, &QLineEdit::returnPressed, this, &TabWidget::searchKeyUpdate);

    mTopLayout->insertLayout(mTopLayout->count()-1, layout);
    mSearchBarCount++;
    updateAdvanceConditions();
}

void TabWidget::removeConditionBar(int index)
{
    //disconnect signals after index search bars
    for(int cur=0; cur<mLayoutList.count(); cur++) {
        disconnect(mAddButtonList[cur], &QPushButton::clicked, this, &TabWidget::addNewConditionBar);
        disconnect(mRemoveButtonList[cur], SIGNAL(clicked()), mRemoveMapperList[cur], SLOT(map()));
        disconnect(mRemoveMapperList[cur], SIGNAL(mapped(int)), this, SLOT(removeConditionBar(int)));
    }

    mLayoutList[index]->deleteLater();
    mConditionsList[index]->deleteLater();
    mLinkLabelList[index]->deleteLater();
    mClassifyList[index]->deleteLater();
    mInputList[index]->deleteLater();
    mSearchBarList[index]->deleteLater();
    mAddButtonList[index]->deleteLater();
    mRemoveButtonList[index]->deleteLater();
    mRemoveMapperList[index]->deleteLater();

    mLayoutList.removeAt(index);
    mConditionsList.removeAt(index);
    mLinkLabelList.removeAt(index);
    mClassifyList.removeAt(index);
    mInputList.removeAt(index);
    mSearchBarList.removeAt(index);
    mAddButtonList.removeAt(index);
    mRemoveButtonList.removeAt(index);
    mRemoveMapperList.removeAt(index);

    //reconnect signals after index search bars
    for(int cur=0; cur<mLayoutList.count(); cur++) {
        connect(mAddButtonList[cur], &QPushButton::clicked, this, &TabWidget::addNewConditionBar);
        connect(mRemoveButtonList[cur], SIGNAL(clicked()), mRemoveMapperList[cur], SLOT(map()));
        mRemoveMapperList[cur]->setMapping(mRemoveButtonList[cur], cur);
        connect(mRemoveMapperList[cur], SIGNAL(mapped(int)), this, SLOT(removeConditionBar(int)));
    }
    -- mSearchBarCount;
    updateAdvanceConditions();
}

QStringList TabWidget::getCurrentClassify(int rowCount)
{
    QStringList currentList;
    currentList.clear();
    if (rowCount >= mOptionList.size() - 1) {
        return mFileSizeList;
    }

    switch (rowCount) {
    case 1:
        return mFileTypeList;
    case 2:
        return mFileMtimeList;
    default:
        break;
    }

    return currentList;
}

void TabWidget::updateTrashBarVisible(const QString &uri)
{
    bool visible = false;
    mTrashBarLayout->setContentsMargins(10, 0, 10, 0);
    if (uri.indexOf("trash:///") >= 0)
    {
        visible = true;
        mTrashBarLayout->setContentsMargins(10, 5, 10, 5);
    }

    mTrashBar->setVisible(visible);
    mTrashLabel->setVisible(visible);
    mClearButton->setVisible(visible);
    mRecoverButton->setVisible(visible);
}

void TabWidget::handleZoomLevel(int zoomLevel)
{
    currentPage()->getView()->clearIndexWidget();

    int currentViewZoomLevel = currentPage()->getView()->currentZoomLevel();
    int currentViewMimZoomLevel = currentPage()->getView()->minimumZoomLevel();
    int currentViewMaxZoomLevel = currentPage()->getView()->maximumZoomLevel();
    if (zoomLevel == currentViewZoomLevel) {
        return;
    }

    // save default zoom level
    GlobalSettings::getInstance()->setValue(DEFAULT_VIEW_ZOOM_LEVEL, zoomLevel);

    if (zoomLevel <= currentViewMaxZoomLevel && zoomLevel >= currentViewMimZoomLevel) {
        currentPage()->getView()->setCurrentZoomLevel(zoomLevel);
    } else {
        //check which view to switch.
        auto directoryViewManager = DirectoryViewFactoryManager2::getInstance();
        auto viewId = directoryViewManager->getDefaultViewId(zoomLevel, getCurrentUri());
        switchViewType(viewId);
        currentPage()->getView()->setCurrentZoomLevel(zoomLevel);
    }
}

void TabWidget::updateSearchBar(bool showSearch)
{
    mShowSearchBar = showSearch;
    if (showSearch) {
        mSearchPath->show();
        mSearchClose->show();
        mSearchTitle->show();
        mSearchBar->show();
        mSearchChild->show();
        mSearchMore->show();
        mSearchBarLayout->setContentsMargins(10, 5, 10, 5);
        mSearchMore->setIcon(QIcon::fromTheme("go-down"));
        updateSearchPathButton();
    } else {
        mSearchPath->hide();
        mSearchClose->hide();
        mSearchTitle->hide();
        mSearchBar->hide();
        mSearchChild->hide();
        mSearchMore->hide();
        Q_EMIT this->closeSearch();
        mSearchBarLayout->setContentsMargins(10, 0, 10, 0);
    }

    if (mSearchBarCount >0) {
        updateSearchList();
    }
}

void TabWidget::updateSearchPathButton(const QString &uri)
{
    //search path not update
    if (uri.startsWith("search://")) {
        return;
    }

    QString curUri = uri;
    if (uri == "") {
        curUri = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        if (! getCurrentUri().isNull())
            curUri = getCurrentUri();
    }
    auto iconName = FileUtils::getFileIconName(curUri);
    auto displayName = FileUtils::getFileDisplayName(curUri);
    mSearchPath->setIcon(QIcon::fromTheme(iconName));
    mSearchPath->setText(displayName);
}

void TabWidget::updateSearchList()
{
    mShowSearchList = !mShowSearchList;
    //if not show search bar, then don't show search list
    if (mShowSearchList && mShowSearchBar) {
        mSearchMore->setIcon(QIcon::fromTheme("go-up"));
        //first click to show advance serach
        if(mSearchBarList.count() ==0) {
            addNewConditionBar();
            return;
        }

        //already had a list,just set to show
        for(int i=0; i<mSearchBarList.count(); i++) {
            mConditionsList[i]->show();
            mLinkLabelList[i]->show();
            if (mConditionsList[i]->currentIndex() >0) {
                mClassifyList[i]->show();
            } else {
                mInputList[i]->show();
            }
            mSearchBarList[i]->show();
            mAddButtonList[i]->show();
            mRemoveButtonList[i]->show();
            mLayoutList[i]->setContentsMargins(10, 0, 10, 5);
        }
    } else {
        //hide search list
        mSearchMore->setIcon(QIcon::fromTheme("go-down"));
        for(int i=0; i<mSearchBarList.count(); i++) {
            mConditionsList[i]->hide();
            mLinkLabelList[i]->hide();
            mClassifyList[i]->hide();
            mInputList[i]->hide();
            mSearchBarList[i]->hide();
            mAddButtonList[i]->hide();
            mRemoveButtonList[i]->hide();
            mLayoutList[i]->setContentsMargins(10, 0, 10, 0);
        }
    }
}

DirectoryViewContainer *TabWidget::currentPage()
{
    return qobject_cast<DirectoryViewContainer *>(mStack->currentWidget());
}

const QString TabWidget::getCurrentUri()
{
    return currentPage()->getCurrentUri();
}

const QStringList TabWidget::getCurrentSelections()
{
    return currentPage()->getCurrentSelections();
}

const QStringList TabWidget::getAllFileUris()
{
    return currentPage()->getAllFileUris();
}

const QStringList TabWidget::getBackList()
{
    return currentPage()->getBackList();
}

const QStringList TabWidget::getForwardList()
{
    return currentPage()->getForwardList();
}

bool TabWidget::canGoBack()
{
    return currentPage()->canGoBack();
}

bool TabWidget::canGoForward()
{
    return currentPage()->canGoForward();
}

bool TabWidget::canCdUp()
{
    return currentPage()->canCdUp();
}

int TabWidget::getSortType()
{
    return currentPage()->getSortType();
}

Qt::SortOrder TabWidget::getSortOrder()
{
    return currentPage()->getSortOrder();
}

bool TabWidget::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::Resize) {
        updateStatusBarGeometry();
    }
    Q_UNUSED(obj);
    return false;
}

void TabWidget::setCurrentIndex(int index)
{
    mTabBar->setCurrentIndex(index);
    mStack->setCurrentIndex(index);
}

void TabWidget::setPreviewPage(PreviewPageIface *previewPage)
{
    bool visible = false;
    auto previewPageWidget = dynamic_cast<QWidget *>(previewPage);
    if (previewPageWidget)
        visible = true;

    if (mPreviewPage) {
        mPreviewPageContainer->removeWidget(mPreviewPageContainer->widget(0));
        mPreviewPage->closePreviewPage();
    }

    mPreviewPage = previewPage;

    if (mPreviewPage) {
        previewPageWidget->setParent(mPreviewPageContainer);
        mPreviewPageContainer->addWidget(previewPageWidget);
        updatePreviewPage();
    }

    mPreviewPageContainer->blockSignals(!visible);
    mPreviewPageContainer->setVisible(visible);
}

void TabWidget::addPage(const QString &uri, bool jumpTo)
{
    auto info = FileInfo::fromUri(uri, false);
    if (! info->isDir()) {
        return;
    }

    QCursor c;
    c.setShape(Qt::WaitCursor);
    this->setCursor(c);

    mTabBar->addPage(uri, jumpTo);
    int zoomLevel = -1;

    bool hasCurrentPage = false;

    if (currentPage()) {
        hasCurrentPage = true;
        zoomLevel = currentPage()->getView()->currentZoomLevel();
    }

    auto viewContainer = new DirectoryViewContainer(mStack);
    viewContainer->setSortType(FileItemModel::FileName);
    viewContainer->setSortOrder(Qt::AscendingOrder);

    mStack->addWidget(viewContainer);
    viewContainer->goToUri(uri, false, true);
    if (jumpTo) {
        mStack->setCurrentWidget(viewContainer);
    }

    bindContainerSignal(viewContainer);
    updateTrashBarVisible(uri);

    if (hasCurrentPage) {
        // perfer to use current page view type
        auto internalViews = DirectoryViewFactoryManager2::getInstance()->internalViews();
        if (internalViews.contains(currentPage()->getView()->viewId())) {
            viewContainer->switchViewType(currentPage()->getView()->viewId());
        }
    }

    if (zoomLevel > 0) {
        viewContainer->getView()->setCurrentZoomLevel(zoomLevel);
    } else {
        viewContainer->getView()->setCurrentZoomLevel(GlobalSettings::getInstance()->getValue(DEFAULT_VIEW_ZOOM_LEVEL).toInt());
    }
}

void TabWidget::goToUri(const QString &uri, bool addHistory, bool forceUpdate)
{
    if (! uri.startsWith("search://")) {
        mLastNonSearchPath = uri;
    }
    currentPage()->goToUri(uri, addHistory, forceUpdate);
    mTabBar->updateLocation(mTabBar->currentIndex(), uri);
    updateTrashBarVisible(uri);
}

void TabWidget::updateTabPageTitle()
{
    mTabBar->updateLocation(mTabBar->currentIndex(), getCurrentUri());
    updateTrashBarVisible(getCurrentUri());
}

void TabWidget::switchViewType(const QString &viewId)
{
    if (currentPage()->getView()->viewId() == viewId) {
        return;
    }

    currentPage()->switchViewType(viewId);

    // change default view id
    auto factoryManager = DirectoryViewFactoryManager2::getInstance();
    auto internalViews = factoryManager->internalViews();
    if (internalViews.contains(viewId)) {
        GlobalSettings::getInstance()->setValue(DEFAULT_VIEW_ID, viewId);
    }

    bool supportZoom = this->currentPage()->getView()->supportZoom();
    this->mStatusBar->mSlider->setEnabled(this->currentPage()->getView()->supportZoom());
}

void TabWidget::goBack()
{
    currentPage()->goBack();
}

void TabWidget::goForward()
{
    currentPage()->goForward();
}

void TabWidget::cdUp()
{
    currentPage()->cdUp();
}

void TabWidget::refresh()
{
    currentPage()->refresh();
}

void TabWidget::stopLoading()
{
    currentPage()->stopLoading();
}

void TabWidget::tryJump(int index)
{
    currentPage()->tryJump(index);
}

void TabWidget::clearHistory()
{
    currentPage()->clearHistory();
}

void TabWidget::setSortType(int type)
{
    currentPage()->setSortType(FileItemModel::ColumnType(type));
}

void TabWidget::setSortOrder(Qt::SortOrder order)
{
    currentPage()->setSortOrder(order);
}

void TabWidget::setSortFilter(int FileTypeIndex, int FileMTimeIndex, int FileSizeIndex)
{
    currentPage()->setSortFilter(FileTypeIndex, FileMTimeIndex, FileSizeIndex);
}

void TabWidget::setShowHidden(bool showHidden)
{
    currentPage()->setShowHidden(showHidden);
}

void TabWidget::setUseDefaultNameSortOrder(bool use)
{
    currentPage()->setUseDefaultNameSortOrder(use);
}

void TabWidget::setSortFolderFirst(bool folderFirst)
{
    currentPage()->setSortFolderFirst(folderFirst);
}

void TabWidget::addFilterCondition(int option, int classify, bool updateNow)
{
    currentPage()->addFilterCondition(option, classify, updateNow);
}

void TabWidget::removeFilterCondition(int option, int classify, bool updateNow)
{
    currentPage()->removeFilterCondition(option, classify, updateNow);
}

void TabWidget::clearConditions()
{
    currentPage()->clearConditions();
}

void TabWidget::updateFilter()
{
    currentPage()->updateFilter();
}

void TabWidget::updateAdvanceConditions()
{
    clearConditions();
    for(int i=0; i<mLayoutList.count(); i++)
    {
        addFilterCondition(mConditionsList[i]->currentIndex(), mClassifyList[i]->currentIndex());
    }
    updateFilter();
}

void TabWidget::setCurrentSelections(const QStringList &uris)
{
    currentPage()->getView()->setSelections(uris);
}

void TabWidget::editUri(const QString &uri)
{
    currentPage()->getView()->editUri(uri);
}

void TabWidget::editUris(const QStringList &uris)
{
    currentPage()->getView()->editUris(uris);
}

void TabWidget::onViewDoubleClicked(const QString &uri)
{
    auto info = FileInfo::fromUri(uri, false);
    if (info->uri().startsWith("trash://")) {
        auto w = new PropertiesWindow(QStringList()<<uri);
        w->show();
        return;
    }
    if (info->isDir() || info->isVolume() || info->isVirtual()) {
        Q_EMIT this->updateWindowLocationRequest(uri, true);
    } else {
        FileLaunchManager::openAsync(uri, false, false);
    }
}

void TabWidget::changeCurrentIndex(int index)
{
    mTabBar->setCurrentIndex(index);
    mStack->setCurrentIndex(index);
    Q_EMIT currentIndexChanged(index);
    Q_EMIT activePageChanged();
}

int TabWidget::count()
{
    return mStack->count();
}

int TabWidget::currentIndex()
{
    return mStack->currentIndex();
}

void TabWidget::moveTab(int from, int to)
{
    auto w = mStack->widget(from);
    if (!w) {
        return;
    }
    mStack->removeWidget(w);
    mStack->insertWidget(to, w);
    Q_EMIT tabMoved(from, to);
}

void TabWidget::removeTab(int index)
{
    mTabBar->removeTab(index);
    auto widget = mStack->widget(index);
    mStack->removeWidget(widget);
    widget->deleteLater();
    if (mStack->count() > 0) {
        Q_EMIT activePageChanged();
    }
}

void TabWidget::bindContainerSignal(DirectoryViewContainer *container)
{
    connect(container, &DirectoryViewContainer::updateWindowLocationRequest, this, &TabWidget::updateWindowLocationRequest);
    connect(container, &DirectoryViewContainer::directoryChanged, this, &TabWidget::activePageLocationChanged);
    connect(container, &DirectoryViewContainer::selectionChanged, this, &TabWidget::activePageSelectionChanged);
    connect(container, &DirectoryViewContainer::viewTypeChanged, this, &TabWidget::activePageViewTypeChanged);
    connect(container, &DirectoryViewContainer::viewDoubleClicked, this, &TabWidget::onViewDoubleClicked);
    connect(container, &DirectoryViewContainer::menuRequest, this, &TabWidget::menuRequest);
    connect(container, &DirectoryViewContainer::zoomRequest, this, &TabWidget::zoomRequest);
    connect(container, &DirectoryViewContainer::setZoomLevelRequest, mStatusBar, &TabStatusBar::updateZoomLevelState);
    connect(container, &DirectoryViewContainer::updateStatusBarSliderStateRequest, this, [=]() {
        bool enable = currentPage()->getView()->supportZoom();
        mStatusBar->mSlider->setEnabled(enable);
        mStatusBar->mSlider->setVisible(enable);
    });
}

void TabWidget::updatePreviewPage()
{
    if (!mPreviewPage) {
        return;
    }
    auto selection = getCurrentSelections();
    mPreviewPage->cancel();
    if (selection.isEmpty()) {
        return ;
    }
    mPreviewPage->prepare(selection.first());
    mPreviewPage->startPreview();
}

void TabWidget::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    updateTabBarGeometry();
    updateStatusBarGeometry();
}

void TabWidget::updateTabBarGeometry()
{
    mTabBar->setGeometry(0, 4, mTabBarBg->width(), mTabBar->height());
    mTabBarBg->setFixedHeight(mTabBar->height());
    mTabBar->raise();
}

void TabWidget::updateStatusBarGeometry()
{
    auto font = qApp->font();
    QFontMetrics fm(font);
    mStatusBar->setGeometry(0, this->height() - fm.height() - 10, mStack->width(), fm.height() + 10);
    mStatusBar->raise();
}

const QList<std::shared_ptr<FileInfo>> TabWidget::getCurrentSelectionFileInfos()
{
    const QStringList uris = getCurrentSelections();
    QList<std::shared_ptr<FileInfo>> infos;
    for(auto uri : uris) {
        auto info = FileInfo::fromUri(uri);
        infos << info;
    }
    return infos;
}

PreviewPageContainer::PreviewPageContainer(QWidget *parent) : QStackedWidget(parent)
{

}

PreviewPageButtonGroups::PreviewPageButtonGroups(QWidget *parent) : QButtonGroup(parent)
{
    setExclusive(true);
}

QTabBar *TabWidget::tabBar()
{
    return mTabBar;
}

bool TabWidget::getTriggeredPreviewPage()
{
    return mTriggeredPreviewPage;
}

void TabWidget::setTriggeredPreviewPage(bool trigger)
{
    mTriggeredPreviewPage = trigger;
}
