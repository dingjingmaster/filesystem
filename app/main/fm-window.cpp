#include "fm-window-factory.h"
#include "fm-window.h"

#include <memory>
#include <QApplication>
#include <global-settings.h>
#include <QStandardPaths>
#include <tab-page.h>
#include <sidebar.h>
#include <QVBoxLayout>
#include <memory>
#include <statusbar.h>
#include <file-operation-utils.h>
#include <QMessageBox>
#include <directory-view-container.h>
#include <QUrl>
#include <QDesktopServices>
#include <directory-view-widget.h>
#include <QDir>
#include <qevent.h>

#include <view-factory/directory-view-factory-manager.h>
#include <QPainter>

#include <file/file-operation-manager.h>

#include <window/properties-window.h>

#include <vfs/search-vfs-uri-parser.h>

#include <plugin-iface/preview-page-plugin-iface.h>

FMWindow::FMWindow(const QString &uri, QWidget *parent) : QMainWindow (parent)
{
    auto settings = GlobalSettings::getInstance();
    mShowHiddenFile = settings->isExist("show-hidden")? settings->getValue("show-hidden").toBool(): false;
    mUseDefaultNameSortOrder = settings->isExist("chinese-first")? settings->getValue("chinese-first").toBool(): false;
    mFolderFirst = settings->isExist("folder-first")? settings->getValue("folder-first").toBool(): true;

    setWindowIcon(QIcon::fromTheme("system-file-manager"));
    setWindowTitle(tr("File Manager"));

    mOperationMinimumInterval.setSingleShot(true);

    connect(qApp, &QApplication::paletteChanged, this, [=]() {
        this->repaint();
        for (auto child : this->children()) {
            QWidget *widget = qobject_cast<QWidget*>(child);
            if (widget) {
                widget->repaint();
            }
        }
    });

    setStyleSheet(".FMWindow::separator{border:0; padding:0}");
    setAttribute(Qt::WA_DeleteOnClose);
    setAnimated(false);
    /*
    QTimer::singleShot(1000, [=](){
        QDockWidget *d = new QDockWidget(this);
        //d->setFeatures(QDockWidget::DockWidgetClosable);
        auto l = new QLabel(tr("test"), d);
        d->setTitleBarWidget(l);
        //d->setWidget(new QLabel(tr("test"), d));
        addDockWidget(Qt::TopDockWidgetArea, d);
        QTimer::singleShot(1000, [=](){
            d->hide();
            QTimer::singleShot(1000, [=](){
                d->show();
            });
        });
    });
     */

    auto location = uri;
    if (uri.isNull()) {
        location = "file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    //init to solve second search in empty path issue
    mLastNonSearchLocation = location;

    mSplitter = new QSplitter(Qt::Horizontal, this);
    mSplitter->setChildrenCollapsible(false);
    mSplitter->setLayoutDirection(Qt::LeftToRight);
    mSplitter->setHandleWidth(1);
    mSplitter->setStyleSheet("QSplitter"
                              "{"
                              "border: 0;"
                              "padding: 0;"
                              "margin: 0;"
                              "}");

    setCentralWidget(mSplitter);

    mTab = new TabPage(this);
    mTab->addPage(location);

    mSideBar = new SideBar(this);
//    mFilterBar = new AdvanceSearchBar(this);
//    mFilterBar->setMinimumWidth(180);
//    mFilter = dynamic_cast<QWidget*>(mFilterBar);

    mSideBarContainer = new QStackedWidget(this);
    mSideBarContainer->addWidget(mSideBar);
//    mSideBarContainer->addWidget(mFilter);
    mSideBarContainer->setCurrentWidget(mSideBar);
    mSplitter->addWidget(mSideBarContainer);

    mSplitter->addWidget(mTab);
    mSplitter->setStretchFactor(0, 0);
    mSplitter->setStretchFactor(1, 1);
    mSplitter->setStretchFactor(2, 1);

//    mToolBar = new ToolBar(this, this);
//    mToolBar->setContentsMargins(0, 0, 0, 0);

//    mSearchBar = new SearchBar(this);
//    mSearchBar->setMinimumWidth(200);
    mAdvancedButton = new QPushButton(tr("advanced search"), nullptr);
    mAdvancedButton->setFixedWidth(110);
    mAdvancedButton->setStyleSheet("color: rgb(10,10,255)");
    mClearRecord = new QPushButton(tr("clear record"), nullptr);
    mClearRecord->setFixedWidth(110);
    mClearRecord->setDisabled(true);
    //set button hidden, function not open to public yet
    mAdvancedButton->setVisible(false);
    mClearRecord->setVisible(false);

    mPreviewPageContainer = new PreviewPageContainer(this);
    mSplitter->addWidget(mPreviewPageContainer);
    mPreviewPageContainer->hide();

    //put the tool bar and search bar into
    //a hobx-layout widget, and put the widget int a
    //new tool bar.
    QWidget *w1 = new QWidget(this);
    w1->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *l1 = new QHBoxLayout(w1);
    l1->setContentsMargins(5, 0, 0, 0);
    w1->setLayout(l1);
//    l1->addWidget(mToolBar, Qt::AlignLeft);
//    l1->addWidget(mSearchBar, Qt::AlignRight);
    l1->addWidget(mAdvancedButton, Qt::AlignRight);
    l1->addWidget(mClearRecord, Qt::AlignRight);

    ToolBarContainer *t1 = new ToolBarContainer(this);
    t1->setContentsMargins(0, 0, 10, 0);
    t1->setMovable(false);
    t1->addWidget(w1);

    addToolBar(Qt::TopToolBarArea, t1);
    addToolBarBreak();

//    mNavigationBar = new NavigationBar(this);
//    mNavigationBar->setMovable(false);
//    mNavigationBar->bindContainer(mTab->getActivePage());
//    mNavigationBar->updateLocation(location);

    QToolBar *t = new QToolBar(this);
    t->setStyleSheet(".QToolBar{border: 0; padding: 0}");
    t->setMovable(false);
//    t->addWidget(mNavigationBar);
    t->setContentsMargins(0, 0, 0, 0);
    addToolBar(t);

    mStatusBar = new StatusBar(this, this);
    setStatusBar(mStatusBar);

//    mNavigationBar->bindContainer(mTab->getActivePage());

    //connect signals
    connect(mSideBar, &SideBar::updateWindowLocationRequest, this, &FMWindow::slotGoToUri);
    connect(mTab, &TabPage::updateWindowLocationRequest, this, &FMWindow::slotGoToUri);
//    connect(mNavigationBar, &NavigationBar::updateWindowLocationRequest, this, &FMWindow::slotGoToUri);
//    connect(mNavigationBar, &NavigationBar::refreshRequest, this, &FMWindow::slotRefresh);
    connect(mAdvancedButton, &QPushButton::clicked, this, &FMWindow::slotAdvanceSearch);
    connect(mClearRecord, &QPushButton::clicked, this, &FMWindow::slotClearRecord);

    //tab changed
    connect(mTab, &TabPage::currentActiveViewChanged, [=]() {
//        this->mToolBar->updateLocation(getCurrentUri());
//        this->mToolBar->updateStates();
//        this->mNavigationBar->bindContainer(getCurrentPage());
//        this->mNavigationBar->updateLocation(getCurrentUri());
//        this->mStatusBar->update();
        Q_EMIT this->tabPageChanged();
        if (mFilterVisible) {
//            advanceSearch();
//            filterUpdate();
        }
    });

    //location change
    connect(mTab, &TabPage::currentLocationChanged,
            this, &FMWindow::locationChangeEnd);
    connect(this, &FMWindow::locationChangeStart, [=]() {
        mIsLoading = true;
        mSideBar->blockSignals(true);
//        mToolBar->blockSignals(true);
//        mNavigationBar->setBlock(true);
    });
    connect(this, &FMWindow::locationChangeEnd, [=]() {
        mIsLoading = false;
        mSideBar->blockSignals(false);
//        mToolBar->blockSignals(false);
//        mNavigationBar->setBlock(false);
//        mNavigationBar->updateLocation(getCurrentUri());
//        mToolBar->updateLocation(getCurrentUri());
//        mToolBar->updateStates();
    });

    //selection changed
    connect(mTab, &TabPage::currentSelectionChanged, [=]() {
        mStatusBar->update();
//        mToolBar->updateStates();
        Q_EMIT this->windowSelectionChanged();
    });

    //location change
    connect(this, &FMWindow::locationChangeStart, [this]() {
        QCursor c;
        c.setShape(Qt::WaitCursor);
        this->setCursor(c);
        mStatusBar->slotUpdate(tr("Loaing... Press Esc to stop a loading."));
    });

    connect(this, &FMWindow::locationChangeEnd, [this]() {
        QCursor c;
        c.setShape(Qt::ArrowCursor);
        this->setCursor(c);
        mStatusBar->update();
    });

    //view switched
    connect(mTab, &TabPage::viewTypeChanged, [=]() {
//        mToolBar->updateLocation(getCurrentUri());
//        mToolBar->updateStates();
    });

    //search
//    connect(mSearchBar, &SearchBar::searchKeyChanged, [=]() {
//        //FIXME: filter the current directory
//    });
//    connect(mSearchBar, &SearchBar::searchRequest, [=](const QString &key) {
//        QString uri = this->getCurrentUri();
//        if (uri.startsWith("search:///")) {
//            uri = mLastNonSearchLocation;
//        }
//        mUpdateCondition = false; //common search, no filter
//        auto targetUri = SearchVFSUriParser::parseSearchKey(uri, key);
//        this->goToUri(targetUri, true);
//        mClearRecord->setDisabled(false); //has record to clear
//    });

    //action
    QAction *stopLoadingAction = new QAction(this);
    stopLoadingAction->setShortcut(QKeySequence(Qt::Key_Escape));
    addAction(stopLoadingAction);
    connect(stopLoadingAction, &QAction::triggered, this, &FMWindow::slotForceStopLoading);

    //show hidden action
    QAction *showHiddenAction = new QAction(this);
    showHiddenAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    addAction(showHiddenAction);
    connect(showHiddenAction, &QAction::triggered, this, [=]() {
//        this->setShowHidden();
    });

    auto undoAction = new QAction(QIcon::fromTheme("edit-undo-symbolic"), tr("Undo"), this);
    undoAction->setShortcut(QKeySequence::Undo);
    addAction(undoAction);
    connect(undoAction, &QAction::triggered, [=]() {
        FileOperationManager::getInstance()->slotUndo();
    });

    auto redoAction = new QAction(QIcon::fromTheme("edit-redo-symbolic"), tr("Redo"), this);
    redoAction->setShortcut(QKeySequence::Redo);
    addAction(redoAction);
    connect(redoAction, &QAction::triggered, [=]() {
        FileOperationManager::getInstance()->slotRedo();
    });

    auto trashAction = new QAction(this);
    trashAction->setShortcut(Qt::Key_Delete);
    connect(trashAction, &QAction::triggered, [=]() {
        auto uris = this->getCurrentSelections();
        if (!uris.isEmpty()) {
            bool isTrash = this->getCurrentUri() == "trash:///";
            if (!isTrash) {
//                FileOperationUtils::trash(uris, true);
//            } else {
//                FileOperationUtils::executeRemoveActionWithDialog(uris);
            }
        }
    });
    addAction(trashAction);

    auto deleteAction = new QAction(this);
    deleteAction->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete));
    addAction(deleteAction);
    connect(deleteAction, &QAction::triggered, [=]() {
        auto uris = this->getCurrentSelections();
        FileOperationUtils::executeRemoveActionWithDialog(uris);
    });

    auto searchAction = new QAction(this);
    searchAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::CTRL + Qt::Key_F)<<QKeySequence(Qt::CTRL + Qt::Key_E));
    connect(searchAction, &QAction::triggered, this, [=]() {
//        mSearchBar->setFocus();
    });
    addAction(searchAction);

    auto locationAction = new QAction(this);
    locationAction->setShortcuts(QList<QKeySequence>()<<Qt::Key_F4<<QKeySequence(Qt::ALT + Qt::Key_D));
    connect(locationAction, &QAction::triggered, this, [=]() {
//        mNavigationBar->startEdit();
    });
    addAction(locationAction);

    auto newWindowAction = new QAction(this);
    newWindowAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_N));
    connect(newWindowAction, &QAction::triggered, this, [=]() {
        FMWindow *newWindow = new FMWindow(getCurrentUri());
        newWindow->show();
    });
    addAction(newWindowAction);

    auto closeWindowAction = new QAction(this);
    closeWindowAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::ALT + Qt::Key_F4));
    connect(closeWindowAction, &QAction::triggered, this, [=]() {
        this->close();
    });
    addAction(closeWindowAction);

    auto aboutAction = new QAction(this);
    aboutAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F2));
    connect(aboutAction, &QAction::triggered, this, [=]() {
        QMessageBox::about(this,
                           tr("Peony Qt"),
                           tr("Author:\n"
                              "\tYue Lan <lanyue@kylinos.cn>\n"
                              "\tMeihong He <hemeihong@kylinos.cn>\n"
                              "\n"
                              "Copyright (C): 2019-2020, Tianjin KYLIN Information Technology Co., Ltd."));
    });
    addAction(aboutAction);

    auto newTabAction = new QAction(this);
    newTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(newTabAction, &QAction::triggered, this, [=]() {
        this->slotAddNewTabs(QStringList()<<this->getCurrentUri());
    });
    addAction(newTabAction);

    auto closeTabAction = new QAction(this);
    closeTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
    connect(closeTabAction, &QAction::triggered, this, [=]() {
        if (mTab->count() <= 1) {
            this->close();
        } else {
            mTab->removeTab(mTab->currentIndex());
        }
    });
    addAction(closeTabAction);

    auto nextTabAction = new QAction(this);
    nextTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Tab));
    connect(nextTabAction, &QAction::triggered, this, [=]() {
        int currentIndex = mTab->currentIndex();
        if (currentIndex + 1 < mTab->count()) {
            mTab->setCurrentIndex(currentIndex + 1);
        } else {
            mTab->setCurrentIndex(0);
        }
    });
    addAction(nextTabAction);

    auto previousTabAction = new QAction(this);
    previousTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab));
    connect(previousTabAction, &QAction::triggered, this, [=]() {
        int currentIndex = mTab->currentIndex();
        if (currentIndex > 0) {
            mTab->setCurrentIndex(currentIndex - 1);
        } else {
            mTab->setCurrentIndex(mTab->count() - 1);
        }
    });
    addAction(previousTabAction);

    auto newFolderAction = new QAction(this);
    newFolderAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));
    connect(newFolderAction, &QAction::triggered, this, [=]() {
        FileCreateTemplOperation op(getCurrentUri(), FileCreateTemplOperation::EmptyFolder, tr("New Folder"));
        op.run();
        auto targetUri = op.target();
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
        QTimer::singleShot(500, this, [=]() {
#else
        QTimer::singleShot(500, [=]() {
#endif
//            this->getCurrentPage()->getView()->scrollToSelection(targetUri);
            this->slotEditUri(targetUri);
        });
    });
    addAction(newFolderAction);

    auto propertiesWindowAction = new QAction(this);
    propertiesWindowAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Return));
    connect(propertiesWindowAction, &QAction::triggered, this, [=]() {
        if (getCurrentSelections().count() >0)
        {
            PropertiesWindow *w = new PropertiesWindow(getCurrentSelections());
            w->show();
        }
    });
    addAction(propertiesWindowAction);

    auto helpAction = new QAction(this);
    helpAction->setShortcut(QKeySequence(Qt::Key_F1));
    connect(helpAction, &QAction::triggered, this, [=]() {
        QUrl url = QUrl("help:ubuntu-kylin-help/files", QUrl::TolerantMode);
        QDesktopServices::openUrl(url);
    });
    addAction(helpAction);

    auto maxAction = new QAction(this);
    maxAction->setShortcut(QKeySequence(Qt::Key_F11));
    connect(maxAction, &QAction::triggered, this, [=]() {
        if (!this->isFullScreen()) {
            this->showFullScreen();
        } else {
            this->showMaximized();
        }
    });
    addAction(maxAction);

    auto previewPageAction = new QAction(this);
    previewPageAction->setShortcuts(QList<QKeySequence>()<<Qt::Key_F3<<QKeySequence(Qt::ALT + Qt::Key_P));
    connect(previewPageAction, &QAction::triggered, this, [=]() {
//        auto lastPreviewPageId = mNavigationBar->getLastPreviewPageId();
//        mNavigationBar->triggerAction(lastPreviewPageId);
    });
    addAction(previewPageAction);

    auto refreshAction = new QAction(this);
    refreshAction->setShortcut(Qt::Key_F5);
    connect(refreshAction, &QAction::triggered, this, [=]() {
        this->slotRefresh();
    });
    addAction(refreshAction);

    //menu
    mTab->connect(mTab, &TabPage::menuRequest, [=]() {
        if (mIsLoading)
            return;
//        DirectoryViewMenu menu(this, nullptr);
//        menu.exec(QCursor::pos());
//        auto urisToEdit = menu.urisToEdit();
//        if (!urisToEdit.isEmpty()) {
//#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
//            QTimer::singleShot(100, this, [=]() {
//#else
//            QTimer::singleShot(100, [=]() {
//#endif
//                this->getCurrentPage()->getView()->scrollToSelection(urisToEdit.first());
//                this->editUri(urisToEdit.first());
//            });
//        }
    });

    //preview page
//    connect(mNavigationBar, &NavigationBar::switchPreviewPageRequest,
//            this, &FMWindow::onPreviewPageSwitch);
    connect(mTab, &TabPage::currentSelectionChanged, [=]() {
        if (mPreviewPageContainer->getCurrentPage()) {
            auto selection = getCurrentSelections();
            if (!selection.isEmpty()) {
//                mPreviewPageContainer->getCurrentPage()->cancel();
//                mPreviewPageContainer->getCurrentPage()->prepare(selection.first());
//                mPreviewPageContainer->getCurrentPage()->startPreview();
//            } else {
//                mPreviewPageContainer->getCurrentPage()->cancel();
            }
        }
    });
    connect(mTab, &TabPage::currentLocationChanged, [=]() {
        if (mPreviewPageContainer->getCurrentPage()) {
//            mPreviewPageContainer->getCurrentPage()->cancel();
        }
    });
    connect(mTab, &TabPage::currentActiveViewChanged, [=]() {
        if (mPreviewPageContainer->getCurrentPage()) {
            auto selection = getCurrentSelections();
            if (selection.isEmpty()) {
//                mPreviewPageContainer->getCurrentPage()->cancel();
//            } else {
//                mPreviewPageContainer->getCurrentPage()->prepare(selection.first());
//                mPreviewPageContainer->getCurrentPage()->startPreview();
            }
        }
    });
}

int FMWindow::getCurrentSortColumn()
{
    return mTab->getActivePage()->getView()->getSortType();
}

const QString FMWindow::getCurrentUri()
{
    if (mTab->getActivePage()) {
        return mTab->getActivePage()->getCurrentUri();
    }
    return nullptr;
}

FMWindowFactory *FMWindow::getFactory()
{
    return FMWindowFactory::getInstance();
}

Qt::SortOrder FMWindow::getCurrentSortOrder()
{
    return Qt::SortOrder(mTab->getActivePage()->getView()->getSortOrder());
}

FMWindowIface *FMWindow::create(const QString &uri)
{
    return new FMWindow(uri);
}

FMWindowIface *FMWindow::create(const QStringList &uris)
{
    if (uris.isEmpty())
        return nullptr;
    auto window = new FMWindow(uris.first());
    QStringList l;
    for (auto uri : uris) {
        if (uris.indexOf(uri) != 0) {
            l<<uri;
        }
    }
    if (!l.isEmpty())
        window->slotAddNewTabs(l);
    return window;
}

const QList<std::shared_ptr<FileInfo>> FMWindow::getCurrentSelectionFileInfos()
{
    const QStringList uris = getCurrentSelections();
    QList<std::shared_ptr<FileInfo>> infos;
    for(auto uri : uris) {
        auto info = FileInfo::fromUri(uri);
        infos<<info;
    }
    return infos;
}

void FMWindow::slotRefresh()
{
    if (mOperationMinimumInterval.isActive()) {
        return;
    }
    mOperationMinimumInterval.start(500);
    if (mFilterVisible) {
//        advanceSearch();
//        filterUpdate();
    }
    //qDebug() << "page refresh:" << getCurrentUri();
    //update page uri before refresh, fix go to root path issue after refresh
    mTab->getActivePage()->getView()->setDirectoryUri(getCurrentUri());
    mTab->getActivePage()->refresh();
}

void FMWindow::slotClearRecord()
{
//    mSearchBar->clearSearchRecord();
    mClearRecord->setDisabled(true);
}

void FMWindow::slotSetShowHidden()
{
    mShowHiddenFile = !mShowHiddenFile;
    mTab->getActivePage()->setShowHidden(mShowHiddenFile);
}

void FMWindow::slotAdvanceSearch()
{
    if (mSideBarContainer->currentWidget() == mFilter)
    {
        //clear filter
//        filterUpdate();
//        mFilterBar->clearData();
        mSideBarContainer->setCurrentWidget(mSideBar);
        mFilterVisible = false;
    }
    else
    {
        mFilterVisible = true;
        //before show, update cur uri
        QString target_path = getCurrentUri();
        if (target_path.contains("file://"))
            mAdvanceTargetPath = target_path;
        else
            mAdvanceTargetPath = "file://" + target_path;

        //set default search path as current path
//        if (mFilterBar) {
//            mFilterBar->setdefaultpath(target_path);
//        }
//        mFilterBar->updateLocation();
//        mSideBarContainer->setCurrentWidget(mFilter);
    }
    //to solve back up automatic pop up issue
    QTimer::singleShot(100, this, [=]() {
//        mSearchBar->hideTableView();
    });
}

void FMWindow::slotForceStopLoading()
{
    mTab->getActivePage()->stopLoading();
    mIsLoading = false;
}

void FMWindow::slotSetSortFolderFirst()
{
    mFolderFirst = ! mFolderFirst;
    getCurrentPage()->setSortFolderFirst(mFolderFirst);
}

void FMWindow::slotEditUri(const QString &uri)
{
    getCurrentPage()->getView()->editUri(uri);

}

void FMWindow::slotSetUseDefaultNameSortOrder()
{
    mUseDefaultNameSortOrder = ! mUseDefaultNameSortOrder;
    getCurrentPage()->setUseDefaultNameSortOrder(mUseDefaultNameSortOrder);
}

void FMWindow::slotEditUris(const QStringList &uris)
{
    getCurrentPage()->getView()->editUris(uris);
}

void FMWindow::slotAddNewTabs(const QStringList &uris)
{
    for (auto uri : uris) {
        mTab->addPage(uri);
    }
}

void FMWindow::slotSetCurrentSortColumn(int sortColumn)
{
//    mTab->getActivePage()->getView()->setSortType(column);
}

void FMWindow::slotBeginSwitchView(const QString &viewId)
{
    mTab->getActivePage()->switchViewType(viewId);
}

void FMWindow::slotOnPreviewPageSwitch(const QString &uri)
{
//    if (id.isNull()) {
//        PreviewPageIface *page = mPreviewPageContainer->getCurrentPage();
//        if (page) {
//            mPreviewPageContainer->removePage(page);
//        }
//    } else {
//        auto oldPage = mPreviewPageContainer->getCurrentPage();
//        PreviewPageIface *page = PreviewPageFactoryManager::getInstance()->getPlugin(id)->createPreviewPage();
//        mPreviewPageContainer->setCurrentPage(page);
//        mPreviewPageContainer->removePage(oldPage);
//        //emit selection changed signal manually for starting a preview with new page.
//        Q_EMIT mTab->currentSelectionChanged();
//    }
}

void FMWindow::slotSetCurrentSortOrder(Qt::SortOrder order)
{
//    mTab->getActivePage()->getView()->setSortOrder(order);
//    mToolBar->updateStates();
}

void FMWindow::slotSetCurrentSelectionUris(const QStringList &uris)
{
//    mTab->getActivePage()->getView()->setSelections(uris);
//    mToolBar->updateStates();
}

void FMWindow::slotFilterUpdate(int type_index, int time_index, int size_index)
{
//    mTab->getActivePage()->setSortFilter(type_index, time_index, size_index);
}

void FMWindow::slotGoToUri(const QString &uri, bool addHistory, bool forceUpdate)
{
    QUrl url(uri);
    auto realUri = uri;
    if (url.scheme().isEmpty()) {
        if (uri.startsWith("/")) {
            realUri = "file://" + uri;
        } else {
            QDir currentDir = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
            currentDir.cd(uri);
            auto absPath = currentDir.absoluteFilePath(uri);
            url = QUrl::fromLocalFile(absPath);

            realUri = url.toDisplayString();
        }
    }

    if (forceUpdate)
        goto update;

    if (getCurrentUri() == realUri)
        return;

update:
    if (!realUri.startsWith("search://")) {
        mLastNonSearchLocation = realUri;
    }
    Q_EMIT locationChangeStart();
//    if (mUpdateCondition)
//        filterUpdate();
    mTab->getActivePage()->goToUri(realUri, addHistory, forceUpdate);
    mTab->refreshCurrentTabText();
//    mNavigationBar->updateLocation(realUri);
//    mToolBar->updateLocation(realUri);
}

void FMWindow::slotSearchFilter(QString target_path, QString keyWord, bool search_file_name, bool search_content)
{
    auto targetUri = SearchVFSUriParser::parseSearchKey(target_path, keyWord, search_file_name, search_content);
    //qDebug()<<"targeturi:"<<targetUri;
    mUpdateCondition = true;
    this->slotGoToUri(targetUri, true);
}

void FMWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        /*if (mNavigationBar->isPathEditing()) {
            mNavigationBar->finishEdit();
        } else*/ {
            auto selections = this->getCurrentSelections();
            if (selections.count() == 1) {
                Q_EMIT mTab->getActivePage()->viewDoubleClicked(selections.first());
            } else {
                QStringList files;
                QStringList dirs;
                for (auto uri : selections) {
                    auto info = FileInfo::fromUri(uri);
                    if (info->isDir() || info->isVolume()) {
                        dirs<<uri;
                    } else {
                        files<<uri;
                    }
                }
                for (auto uri : dirs) {
                    mTab->addPage(uri);
                }
                for (auto uri : files) {
                    Q_EMIT mTab->getActivePage()->viewDoubleClicked(uri);
                }
            }
        }
    }
    return QMainWindow::keyPressEvent(e);
}

void FMWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
}

QSize FMWindow::sizeHint() const
{
    return QSize(800, 600);
}

const QString FMWindow::getCurrentPageViewType()
{
    if (mTab->getActivePage()->getView()) {
        return mTab->getActivePage()->getView()->viewId();
    }
    else {
        return DirectoryViewFactoryManager2::getInstance()->getDefaultViewId();
    }
}

bool FMWindow::getWindowSortFolderFirst()
{
    return mFolderFirst;
}

bool FMWindow::getWindowUseDefaultNameSortOrder()
{
    return mUseDefaultNameSortOrder;
}

bool FMWindow::getWindowShowHidden()
{
    return mShowHiddenFile;
}

const QString FMWindow::getLastNonSearchUri()
{
    return mLastNonSearchLocation;
}

const QStringList FMWindow::getCurrentSelections()
{
    if (mTab->getActivePage()) {
        return mTab->getActivePage()->getCurrentSelections();
    }
    return QStringList();
}

DirectoryViewContainer *FMWindow::getCurrentPage()
{
    return mTab->getActivePage();
}

const QStringList FMWindow::getCurrentAllFileUris()
{
    if (mTab->getActivePage()) {
        return mTab->getActivePage()->getAllFileUris();
    }
    return QStringList();
}

PreviewPageContainer::PreviewPageContainer(QWidget *parent) : QStackedWidget (parent)
{
    setMinimumWidth(300);
}

void PreviewPageContainer::setCurrentPage(PreviewPageIface *page)
{
    if (count() > 0) {
        PreviewPageIface *oldPage = getCurrentPage();
        if (oldPage) {
            removePage(oldPage);
        }
    }

    addWidget(dynamic_cast<QWidget*>(page));
    setCurrentWidget(dynamic_cast<QWidget*>(page));
    dynamic_cast<QWidget*>(page)->show();
    show();
}

void PreviewPageContainer::removePage(PreviewPageIface *page)
{
    if (!page)
        return;
    removeWidget(dynamic_cast<QWidget*>(page));
    if (count() == 0) {
        hide();
    }
    page->closePreviewPage();
}

PreviewPageIface *PreviewPageContainer::getCurrentPage()
{
    return dynamic_cast<PreviewPageIface*>(currentWidget());
}

//ToolBarContainer
ToolBarContainer::ToolBarContainer(QWidget *parent) : QToolBar(parent)
{

}

void ToolBarContainer::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setPen(this->palette().dark().color());
    p.drawLine(this->rect().bottomLeft(), this->rect().bottomRight());
}
