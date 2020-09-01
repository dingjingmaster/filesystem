#include "main-window.h"

#include "properties-window.h"
#include "main-window-factory.h"

#include <QDir>
#include <QTimer>
#include <QSlider>
#include <QAction>
#include <QScreen>
#include <QTabBar>
#include <QScreen>
#include <X11/X.h>
#include <QX11Info>
#include <QPainter>
#include <QTreeView>
#include <QSplitter>
#include <X11/Xlib.h>
#include <QDockWidget>
#include <QMouseEvent>
#include <QMessageBox>
#include <QApplication>
#include <file-utils.h>
#include <QStandardPaths>
#include <tab-statusbar.h>
#include <clipbord-utils.h>
#include <file-label-box.h>
#include <global-settings.h>
#include <syslog/clib_syslog.h>
#include <file-operation-utils.h>
#include <directory-view-widget.h>
#include <main/x11-window-manager.h>
#include <directory-view-container.h>
#include <file/file-operation-manager.h>
#include <previewpage-factory-manager.h>
#include <file/file-operation-error-dialog.h>
#include <QtWidgets/private/qwidgetresizehandler_p.h>
#include <menu/directory-view-menu.h>

static MainWindow* gLastResizeWindow = nullptr;

MainWindow::MainWindow(const QString &uri, QWidget *parent) : QMainWindow(parent)
{
    CT_SYSLOG(LOG_DEBUG, "MainWindow construct ...");
    installEventFilter(this);

    setWindowIcon(QIcon::fromTheme("system-file-manager"));
    setWindowTitle(tr("File Manager"));

    slotCheckSettings();

    setAnimated(false);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    //fix double window base buttons issue, not effect MinMax button hints
    auto flags = windowFlags() & ~Qt::WindowMinMaxButtonsHint;
    setWindowFlags(flags | Qt::FramelessWindowHint);
    setWindowFlags(windowFlags()|Qt::FramelessWindowHint);
    setContentsMargins(4, 4, 4, 4);

    //bind resize handler
    auto handler = new QWidgetResizeHandler(this);
    handler->setMovingEnabled(false);
    mResizeHandler = handler;

    setProperty("useStyleWindowManager", false);

    slotSetShortCuts();

    initUI(uri);
    CT_SYSLOG(LOG_DEBUG, "MainWindow construct ok!");
}

MainWindow::~MainWindow()
{
    if (gLastResizeWindow == this) {
        auto settings = GlobalSettings::getInstance();
        settings->setValue(DEFAULT_WINDOW_SIZE, this->size());
        gLastResizeWindow = nullptr;
    }
}

QSize MainWindow::sizeHint() const
{
    auto screenSize = QApplication::primaryScreen()->size();
    QSize defaultSize = (GlobalSettings::getInstance()->getValue(DEFAULT_WINDOW_SIZE)).toSize();
    int width = qMin(defaultSize.width(), screenSize.width());
    int height = qMin(defaultSize.height(), screenSize.height());
    return QSize(width, height);
}

int MainWindow::getCurrentSortColumn()
{
    return mTab->getSortType();
}

bool MainWindow::getWindowShowHidden()
{
    return mShowHiddenFile;
}

bool MainWindow::getWindowUseDefaultNameSortOrder()
{
    return mUseDefaultNameSortOrder;
}

DirectoryViewContainer *MainWindow::getCurrentPage()
{
    return mTab->currentPage();
}

const QStringList MainWindow::getCurrentSelections()
{
    return mTab->getCurrentSelections();
}

const QStringList MainWindow::getCurrentAllFileUris()
{
    return mTab->getAllFileUris();
}

FMWindowIface *MainWindow::create(const QString &uri)
{
    auto window = new MainWindow(uri);
    if (currentViewSupportZoom()) {
        window->slotSetCurrentViewZoomLevel(this->currentViewZoomLevel());
    }

    return window;
}

FMWindowIface *MainWindow::create(const QStringList &uris)
{
    if (uris.isEmpty()) {
        auto window = new MainWindow;
        if (currentViewSupportZoom()) {
            window->slotSetCurrentViewZoomLevel(this->currentViewZoomLevel());
        }
        return window;
    }
    auto uri = uris.first();
    auto l = uris;
    l.removeAt(0);
    auto window = new MainWindow(uri);
    if (currentViewSupportZoom()) {
        window->slotSetCurrentViewZoomLevel(this->currentViewZoomLevel());
    }

    window->slotAddNewTabs(l);

    return window;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        auto me = static_cast<QMouseEvent *>(event);
        auto widget = this->childAt(me->pos());
        if (!widget) {
            mShouldSaveSideBarWidth = true;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        if (mShouldSaveSideBarWidth) {
            auto settings = GlobalSettings::getInstance();
            settings->setValue(DEFAULT_SIDEBAR_WIDTH, mSideBar->width());
        }
        mShouldSaveSideBarWidth = false;
    }

    return false;
}

FMWindowIface *MainWindow::createWithZoomLevel(const QString &uri, int zoomLevel)
{
    auto window = new MainWindow(uri);

    if (currentViewSupportZoom()) {
        window->slotSetCurrentViewZoomLevel(zoomLevel);
    }

    return window;
}

const QList<std::shared_ptr<FileInfo> > MainWindow::getCurrentSelectionFileInfos()
{
    const QStringList uris = getCurrentSelections();
    QList<std::shared_ptr<FileInfo>> infos;
    for(auto uri : uris) {
        auto info = FileInfo::fromUri(uri);
        infos << info;
    }
    return infos;
}

FMWindowIface *MainWindow::createWithZoomLevel(const QStringList &uris, int zoomLevel)
{
    if (uris.isEmpty()) {
        auto window = new MainWindow;
        if (currentViewSupportZoom())
            window->slotSetCurrentViewZoomLevel(zoomLevel);
        return window;
    }
    auto uri = uris.first();
    auto l = uris;
    l.removeAt(0);
    auto window = new MainWindow(uri);
    if (currentViewSupportZoom()) {
        window->slotSetCurrentViewZoomLevel(zoomLevel);
    }

    window->slotAddNewTabs(l);

    return window;
}

void MainWindow::slotRefresh()
{
    slotGoToUri(getCurrentUri(), false, true);
}

void MainWindow::slotCleanTrash()
{
    auto result = QMessageBox::question(nullptr, tr("Delete Permanently"),
                                        tr("Are you sure that you want to delete these files? "
                                           "Once you start a deletion, the files deleting will never be "
                                           "restored again."));
    if (result == QMessageBox::Yes) {
        auto uris = getCurrentAllFileUris();
        FileOperationUtils::remove(uris);
    }
}

void MainWindow::slotClearRecord()
{

}

void MainWindow::slotSetShortCuts()
{
    //stop loading action
    QAction *stopLoadingAction = new QAction(this);
    stopLoadingAction->setShortcut(QKeySequence(Qt::Key_Escape));
    addAction(stopLoadingAction);
    connect(stopLoadingAction, &QAction::triggered, this, &MainWindow::slotForceStopLoading);

    //show hidden action
    QAction *showHiddenAction = new QAction(this);
    showHiddenAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    addAction(showHiddenAction);
    connect(showHiddenAction, &QAction::triggered, this, [=]() {
        this->slotSetShowHidden();
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

    //add CTRL+D for delete operation
    auto trashAction = new QAction(this);
    trashAction->setShortcuts(QList<QKeySequence>()<<Qt::Key_Delete<<QKeySequence(Qt::CTRL + Qt::Key_D));
    connect(trashAction, &QAction::triggered, [=]() {
        auto uris = this->getCurrentSelections();
        if (!uris.isEmpty()) {
            bool isTrash = this->getCurrentUri() == "trash:///";
            if (!isTrash) {
                FileOperationUtils::trash(uris, true);
            } else {
                FileOperationUtils::executeRemoveActionWithDialog(uris);
            }
        }
    });
    addAction(trashAction);

    auto deleteAction = new QAction(this);
    deleteAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::SHIFT + Qt::Key_Delete));
    addAction(deleteAction);
    connect(deleteAction, &QAction::triggered, [=]() {
        auto uris = this->getCurrentSelections();
        FileOperationUtils::executeRemoveActionWithDialog(uris);
    });

    auto searchAction = new QAction(this);
    searchAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::CTRL + Qt::Key_F));
    connect(searchAction, &QAction::triggered, this, [=]() {
        mIsSearch = ! mIsSearch;
        mHeaderBar->startEdit(mIsSearch);
    });
    addAction(searchAction);

    //F4 or Alt+D, change to address
    auto locationAction = new QAction(this);
    locationAction->setShortcuts(QList<QKeySequence>()<<Qt::Key_F4<<QKeySequence(Qt::ALT + Qt::Key_D));
    connect(locationAction, &QAction::triggered, this, [=]() {
        mHeaderBar->startEdit();
    });
    addAction(locationAction);

    auto newWindowAction = new QAction(this);
    newWindowAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_N));
    connect(newWindowAction, &QAction::triggered, this, [=]() {
        MainWindow *newWindow = new MainWindow(getCurrentUri());
        newWindow->show();
    });
    addAction(newWindowAction);

    auto closeWindowAction = new QAction(this);
    closeWindowAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::ALT + Qt::Key_F4));
    connect(closeWindowAction, &QAction::triggered, this, [=]() {
        this->close();
    });
    addAction(closeWindowAction);

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
    previousTabAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab));
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
    newFolderAction->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));
    connect(newFolderAction, &QAction::triggered, this, &MainWindow::slotCreateFolderOperation);
    addAction(newFolderAction);

    //show selected item's properties
    auto propertiesWindowAction = new QAction(this);
    propertiesWindowAction->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::ALT + Qt::Key_Return) << QKeySequence(Qt::ALT + Qt::Key_Enter));
    connect(propertiesWindowAction, &QAction::triggered, this, [=]() {
        //Fixed issue:when use this shortcut without any selections, this will crash
        if (getCurrentSelections().count() > 0) {
            PropertiesWindow *w = new PropertiesWindow(getCurrentSelections());
            w->show();
        }
    });
    addAction(propertiesWindowAction);

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
        auto triggered = mTab->getTriggeredPreviewPage();
        if (triggered) {
            mTab->setPreviewPage(nullptr);
        } else {
            auto instance = PreviewPageFactoryManager::getInstance();
            auto lastPreviewPageId  = instance->getLastPreviewPageId();
            auto *page = instance->getPlugin(lastPreviewPageId)->createPreviewPage();
            mTab->setPreviewPage(page);
        }
        mTab->setTriggeredPreviewPage(!triggered);
    });
    addAction(previewPageAction);

    // refresh
    auto refreshAction = new QAction(this);
    refreshAction->setShortcut(Qt::Key_F5);
    connect(refreshAction, &QAction::triggered, this, [=]() {
        this->slotRefresh();
    });
    addAction(refreshAction);

    //file operations
    auto *copyAction = new QAction(this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, [=]() {
        if (!this->getCurrentSelections().isEmpty())
            ClipbordUtils::setClipboardFiles(this->getCurrentSelections(), false);
    });
    addAction(copyAction);

    // paste
    auto *pasteAction = new QAction(this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, [=]() {
        if (ClipbordUtils::isClipboardHasFiles()) {
            ClipbordUtils::pasteClipboardFiles(this->getCurrentUri());
        }
    });
    addAction(pasteAction);

    // cut
    auto *cutAction = new QAction(this);
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, [=]() {
        if (!this->getCurrentSelections().isEmpty()) {
            ClipbordUtils::setClipboardFiles(this->getCurrentSelections(), true);
        }
    });
    addAction(cutAction);
}

void MainWindow::slotAdvanceSearch()
{
    initAdvancePage();
}

void MainWindow::slotSetShowHidden()
{
    mShowHiddenFile = !mShowHiddenFile;
    getCurrentPage()->setShowHidden(mShowHiddenFile);
}

void MainWindow::slotCheckSettings()
{
    auto settings = GlobalSettings::getInstance();
    mShowHiddenFile = settings->isExist("show-hidden")? settings->getValue("show-hidden").toBool(): false;
    mUseDefaultNameSortOrder = settings->isExist("chinese-first")? settings->getValue("chinese-first").toBool(): false;
    mFolderFirst = settings->isExist("folder-first")? settings->getValue("folder-first").toBool(): true;
}

void MainWindow::slotUpdateHeaderBar()
{
    mHeaderBar->setLocation(getCurrentUri());
    mHeaderBar->updateIcons();
}

void MainWindow::slotForceStopLoading()
{
    mTab->stopLoading();
}

void MainWindow::slotRecoverFromTrash()
{
    auto selections = getCurrentSelections();
    if (selections.isEmpty()) {
        selections = getCurrentAllFileUris();
    }

    if (selections.count() == 1) {
        FileOperationUtils::restore(selections.first());
    } else {
        FileOperationUtils::restore(selections);
    }
}

void MainWindow::slotMaximizeOrRestore()
{
    if (!this->isMaximized()) {
        this->showMaximized();
    } else {
        this->showNormal();
    }
    mHeaderBar->updateIcons();
}

void MainWindow::slotSetSortFolderFirst()
{
    mFolderFirst = ! mFolderFirst;
    getCurrentPage()->setSortFolderFirst(mFolderFirst);
}

void MainWindow::slotUpdateTabPageTitle()
{
    mTab->updateTabPageTitle();
    auto show = FileUtils::getFileDisplayName(getCurrentUri());
    QString title = show + "-" + tr("File Manager");
    setWindowTitle(title);
}

void MainWindow::slotCreateFolderOperation()
{
    FileCreateTemplOperation op(getCurrentUri(), FileCreateTemplOperation::EmptyFolder, tr("New Folder"));
    FileOperationErrorDialog dlg;
    connect(&op, &FileOperation::errored, &dlg, &FileOperationErrorDialog::slotHandleError);
    op.run();
    auto targetUri = op.target();
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
    QTimer::singleShot(500, this, [=]() {
#else
    QTimer::singleShot(500, [=]() {
#endif
        this->getCurrentPage()->getView()->scrollToSelection(targetUri);
        this->slotEditUri(targetUri);
    });
}

void MainWindow::slotEditUri(const QString &uri)
{
    mTab->editUri(uri);
}

void MainWindow::slotSetUseDefaultNameSortOrder()
{
    mUseDefaultNameSortOrder = ! mUseDefaultNameSortOrder;
    getCurrentPage()->setUseDefaultNameSortOrder(mUseDefaultNameSortOrder);
}

void MainWindow::slotSetLabelNameFilter(QString name)
{
    getCurrentPage()->setFilterLabelConditions(name);
}

void MainWindow::slotEditUris(const QStringList &uris)
{
    mTab->editUris(uris);
}

void MainWindow::slotAddNewTabs(const QStringList &uris)
{
    for (auto uri : uris) {
        mTab->addPage(uri, false);
    }
}

void MainWindow::slotSetCurrentSortColumn(int sortColumn)
{
    mTab->setSortType(sortColumn);
}

void MainWindow::slotSetCurrentViewZoomLevel(int zoomLevel)
{
    if (currentViewSupportZoom()) {
        mTab->mStatusBar->mSlider->setValue(zoomLevel);
    }
}

void MainWindow::slotBeginSwitchView(const QString &viewId)
{
    auto selection = getCurrentSelections();
    mTab->switchViewType(viewId);
    // save zoom level
    GlobalSettings::getInstance()->setValue(DEFAULT_VIEW_ZOOM_LEVEL, currentViewZoomLevel());
    mTab->setCurrentSelections(selection);
    mTab->mStatusBar->mSlider->setEnabled(mTab->currentPage()->getView()->supportZoom());
}

void MainWindow::slotSyncControlsLocation(const QString &uri)
{
    mTab->goToUri(uri, false, false);
}

void MainWindow::slotSetCurrentSortOrder(Qt::SortOrder order)
{
    mTab->setSortOrder(order);
}

void MainWindow::slotSetCurrentSelectionUris(const QStringList &uris)
{
    mTab->setCurrentSelections(uris);
}

void MainWindow::slotFilterUpdate(int type_index, int time_index, int size_index)
{

}

void MainWindow::slotGoToUri(const QString &uri, bool addHistory, bool force)
{
    CT_SYSLOG(LOG_DEBUG, "go to uri: %s", uri.toUtf8().constData());
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

    if (getCurrentUri() == realUri) {
        if (!force) {
            return;
        }
    }

    CT_SYSLOG(LOG_DEBUG, "real uri: %s locationChangeStart!", realUri.toUtf8().constData());
    locationChangeStart();
    CT_SYSLOG(LOG_DEBUG, "real uri: %s locationChangeStart!", realUri.toUtf8().constData());
    mTab->goToUri(realUri, addHistory, force);
    CT_SYSLOG(LOG_DEBUG, "real uri: %s locationChangeStart!", realUri.toUtf8().constData());
    mHeaderBar->setLocation(uri);
}

void MainWindow::slotSearchFilter(QString targetPath, QString keyWord, bool search_file_name, bool search_content)
{

}

void MainWindow::validBorder()
{
    if (this->isMaximized()) {
        setContentsMargins(0, 0, 0, 0);
//        mEffect->setPadding(0);
        setProperty("blurRegion", QVariant());
    } else {
        setContentsMargins(4, 4, 4, 4);
//        mEffect->setPadding(4);
        QPainterPath path;
        auto rect = this->rect();
        rect.adjust(4, 4, -4, -4);
        path.addRoundedRect(rect, 6, 6);
        setProperty("blurRegion", QRegion(path.toFillPolygon().toPolygon()));
    }
}

QRect MainWindow::sideBarRect()
{
    auto pos = mTransparentAreaWidget->mapTo(this, QPoint());
    return QRect(pos, mTransparentAreaWidget->size());
}

void MainWindow::initAdvancePage()
{

}

void MainWindow::initUI(const QString &uri)
{
    connect(this, &MainWindow::locationChangeStart, this, [=]() {
        mSideBar->blockSignals(true);
        mHeaderBar->blockSignals(true);
        QCursor c;
        c.setShape(Qt::WaitCursor);
        this->setCursor(c);
        mTab->setCursor(c);
        mSideBar->setCursor(c);
//        mStatusBar->update();
    });

    connect(this, &MainWindow::locationChangeEnd, this, [=]() {
        mSideBar->blockSignals(false);
        mHeaderBar->blockSignals(false);
        QCursor c;
        c.setShape(Qt::ArrowCursor);
        this->setCursor(c);
        mTab->setCursor(c);
        mSideBar->setCursor(c);
        slotUpdateHeaderBar();
//        mStatusBar->update();
    });

    //HeaderBar
    auto headerBar = new HeaderBar(this);
    mHeaderBar = headerBar;
    auto headerBarContainer = new HeaderBarContainer(this);
    headerBarContainer->addHeaderBar(headerBar);                    // 最大化、最小化、关闭
    addToolBar(headerBarContainer);
//    mHeaderBar->setVisible(false);

    connect(mHeaderBar, &HeaderBar::updateLocationRequest, this, &MainWindow::slotGoToUri);
    connect(mHeaderBar, &HeaderBar::viewTypeChangeRequest, this, &MainWindow::slotBeginSwitchView);
    connect(mHeaderBar, &HeaderBar::updateZoomLevelHintRequest, this, [=](int zoomLevelHint) {
        if (zoomLevelHint >= 0) {
            mTab->mStatusBar->mSlider->setEnabled(true);
            mTab->mStatusBar->mSlider->setValue(zoomLevelHint);
        } else {
            mTab->mStatusBar->mSlider->setEnabled(false);
        }
    });

    //SideBar
    QDockWidget *sidebarContainer = new QDockWidget(this);

    sidebarContainer->setFeatures(QDockWidget::NoDockWidgetFeatures);
    auto palette = sidebarContainer->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    sidebarContainer->setPalette(palette);
//    sidebarContainer->setStyleSheet("{"
//                                    "background-color: transparent;"
//                                    "border: 0px solid transparent"
//                                    "}");
    sidebarContainer->setTitleBarWidget(new QWidget(this));
    sidebarContainer->titleBarWidget()->setFixedHeight(0);
    sidebarContainer->setAttribute(Qt::WA_TranslucentBackground);
    sidebarContainer->setContentsMargins(0, 0, 0, 0);

    NavigationSideBar *sidebar = new NavigationSideBar(this);
    mSideBar = sidebar;

    auto navigationSidebarContainer = new NavigationSideBarContainer(this);
    navigationSidebarContainer->addSideBar(mSideBar);

    CT_SYSLOG(LOG_ERR, "1");
    mTransparentAreaWidget = navigationSidebarContainer;

    connect(mSideBar, &NavigationSideBar::updateWindowLocationRequest, this, &MainWindow::slotGoToUri);

    auto labelDialog = new FileLabelBox(this);
    labelDialog->hide();

    auto splitter = new QSplitter(this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(0);
    splitter->addWidget(navigationSidebarContainer);
    splitter->addWidget(labelDialog);

    connect(labelDialog->selectionModel(), &QItemSelectionModel::selectionChanged, [=]() {
        auto selected = labelDialog->selectionModel()->selectedIndexes();
        //qDebug() << "FileLabelBox selectionChanged:" <<selected.count();
        if (selected.count() > 0) {
            auto name = selected.first().data().toString();
            slotSetLabelNameFilter(name);
        }
    });

    connect(labelDialog, &FileLabelBox::leftClickOnBlank, [=]() {
        slotSetLabelNameFilter("");
    });

    connect(sidebar, &NavigationSideBar::labelButtonClicked, labelDialog, &QWidget::setVisible);

    sidebarContainer->setWidget(splitter);
    addDockWidget(Qt::LeftDockWidgetArea, sidebarContainer);

    mStatusBar = new StatusBar(this, this);
    setStatusBar(mStatusBar);

    auto views = new TabWidget;
    mTab = views;
    if (uri.isNull()) {
        auto home = "file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
//        mTab->addPage(home, true);
    } else {
//        mTab->addPage(uri, true);
    }

    connect(views->tabBar(), &QTabBar::tabBarDoubleClicked, this, [=](int index) {
        if (index == -1) {
            slotMaximizeOrRestore();
        }
    });
    connect(views, &TabWidget::closeWindowRequest, this, &QWidget::close);
    connect(mHeaderBar, &HeaderBar::updateSearchRequest, mTab, &TabWidget::updateSearchBar);

    X11WindowManager *tabBarHandler = X11WindowManager::getInstance();
    tabBarHandler->registerWidget(views->tabBar());

    setCentralWidget(views);

    //bind signals
    connect(mTab, &TabWidget::closeSearch, headerBar, &HeaderBar::closeSearch);
    connect(mTab, &TabWidget::clearTrash, this, &MainWindow::slotCleanTrash);
    connect(mTab, &TabWidget::recoverFromTrash, this, &MainWindow::slotRecoverFromTrash);
    connect(mTab, &TabWidget::updateWindowLocationRequest, this, &MainWindow::slotGoToUri);
    connect(mTab, &TabWidget::activePageLocationChanged, this, &MainWindow::locationChangeEnd);
    connect(mTab, &TabWidget::activePageViewTypeChanged, this, &MainWindow::slotUpdateHeaderBar);
    connect(mTab, &TabWidget::activePageChanged, this, &MainWindow::slotUpdateHeaderBar);
    connect(mTab, &TabWidget::activePageChanged, this, [=](){
//        slotSetCurrentViewZoomLevel(currentViewZoomLevel());
    });
    connect(mTab, &TabWidget::menuRequest, this, [=]() {
        DirectoryViewMenu menu(this);
        menu.exec(QCursor::pos());
        auto urisToEdit = menu.urisToEdit();
        if (!urisToEdit.isEmpty()) {
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
            QTimer::singleShot(100, this, [=]() {
#else
            QTimer::singleShot(100, [=]() {
#endif
                this->getCurrentPage()->getView()->scrollToSelection(urisToEdit.first());
                this->slotEditUri(urisToEdit.first());
            });
        }
    });
}

void MainWindow::paintEvent(QPaintEvent *e)
{
    validBorder();
    QColor color = this->palette().window().color();
    QColor colorBase = this->palette().base().color();

    int R1 = color.red();
    int G1 = color.green();
    int B1 = color.blue();
    qreal a1 = 0.3;

    int R2 = colorBase.red();
    int G2 = colorBase.green();
    int B2 = colorBase.blue();
    qreal a2 = 1;

    qreal a = 1 - (1 - a1)*(1 - a2);

    qreal R = (a1*R1 + (1 - a1)*a2*R2) / a;
    qreal G = (a1*G1 + (1 - a1)*a2*G2) / a;
    qreal B = (a1*B1 + (1 - a1)*a2*B2) / a;

    colorBase.setRed(R);
    colorBase.setGreen(G);
    colorBase.setBlue(B);

    auto sidebarOpacity = GlobalSettings::getInstance()->getValue(SIDEBAR_BG_OPACITY).toInt();

    colorBase.setAlphaF(sidebarOpacity/100.0);

    if (qApp->property("blurEnable").isValid()) {
        bool blurEnable = qApp->property("blurEnable").toBool();
        if (!blurEnable) {
            colorBase.setAlphaF(1);
        }
    } else {
        colorBase.setAlphaF(1);
    }

    QPainterPath sidebarPath;
    sidebarPath.setFillRule(Qt::FillRule::WindingFill);
    auto adjustedRect = sideBarRect().adjusted(0, 1, 0, 0);
    sidebarPath.addRoundedRect(adjustedRect, 6, 6);
    sidebarPath.addRect(adjustedRect.adjusted(0, 0, 0, -6));
    sidebarPath.addRect(adjustedRect.adjusted(6, 0, 0, 0));
//    mEffect->setTransParentPath(sidebarPath);
//    mEffect->setTransParentAreaBg(colorBase);

    color.setAlphaF(0.5);
//    mEffect->setWindowBackground(color);
    QPainter p(this);

//    mEffect->drawWindowShadowManually(&p, this->rect(), m_resize_handler->isButtonDown());
    QMainWindow::paintEvent(e);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Backspace) {
        auto uri = FileUtils::getParentUri(getCurrentUri());
        if (uri.isNull())
            return;
        mTab->goToUri(uri, true, true);
    }

    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        auto selections = this->getCurrentSelections();
        if (selections.count() == 1) {
            Q_EMIT mTab->currentPage()->viewDoubleClicked(selections.first());
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
                Q_EMIT mTab->currentPage()->viewDoubleClicked(uri);
            }
        }
    }

    return QMainWindow::keyPressEvent(e);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    mHeaderBar->updateMaximizeState();
    validBorder();
    update();

    if (mResizeHandler->isButtonDown()) {
        // set save window size flag
        gLastResizeWindow = this;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    //NOTE: when starting a X11 window move, the mouse move event
    //will unreachable when draging, and after draging we could not
    //get the release event correctly.
    QMainWindow::mouseMoveEvent(e);
    if (!mIsDraging) {
        return;
    }

    if (QX11Info::isPlatformX11()) {
        Display *display = QX11Info::display();
        Atom netMoveResize = XInternAtom(display, "_NET_WM_MOVERESIZE", False);
        XEvent xEvent;
        const auto pos = QCursor::pos();

        memset(&xEvent, 0, sizeof(XEvent));
        xEvent.xclient.type = ClientMessage;
        xEvent.xclient.message_type = netMoveResize;
        xEvent.xclient.display = display;
        xEvent.xclient.window = this->winId();
        xEvent.xclient.format = 32;
        xEvent.xclient.data.l[0] = pos.x();
        xEvent.xclient.data.l[1] = pos.y();
        xEvent.xclient.data.l[2] = 8;
        xEvent.xclient.data.l[3] = Button1;
        xEvent.xclient.data.l[4] = 0;

        XUngrabPointer(display, CurrentTime);
        XSendEvent(display, QX11Info::appRootWindow(QX11Info::appScreen()),
                   False, SubstructureNotifyMask | SubstructureRedirectMask,
                   &xEvent);
        XFlush(display);

        XEvent xevent;
        memset(&xevent, 0, sizeof(XEvent));

        xevent.type = ButtonRelease;
        xevent.xbutton.button = Button1;
        xevent.xbutton.window = this->winId();
        xevent.xbutton.x = e->pos().x();
        xevent.xbutton.y = e->pos().y();
        xevent.xbutton.x_root = pos.x();
        xevent.xbutton.y_root = pos.y();
        xevent.xbutton.display = display;

        XSendEvent(display, this->effectiveWinId(), False, ButtonReleaseMask, &xevent);
        XFlush(display);

        if (e->source() == Qt::MouseEventSynthesizedByQt) {
            if (!this->mouseGrabber()) {
                this->grabMouse();
                this->releaseMouse();
            }
        }

        mIsDraging = false;
    } else {
        this->move(QCursor::pos() - mOffset);
    }
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    QMainWindow::mousePressEvent(e);
    if (e->button() == Qt::LeftButton && !e->isAccepted()) {
        mIsDraging = true;
        mOffset = mapFromGlobal(QCursor::pos());
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{
    /*!
     * \bug
     * release event sometimes "disappear" when we request
     * X11 window manager for movement.
     */
    QMainWindow::mouseReleaseEvent(e);
    CT_SYSLOG(LOG_DEBUG, "mouse release!");
    mIsDraging = false;
}


bool MainWindow::getWindowSortFolderFirst()
{
    return mFolderFirst;
}

Qt::SortOrder MainWindow::getCurrentSortOrder()
{
    return mTab->getSortOrder();
}

int MainWindow::currentViewZoomLevel()
{
    if (getCurrentPage()) {
        if (auto view = getCurrentPage()->getView()) {
            return view->currentZoomLevel();
        }
    }

    int defaultZoomLevel = GlobalSettings::getInstance()->getValue(DEFAULT_VIEW_ZOOM_LEVEL).toInt();

    if (defaultZoomLevel >= 0 && defaultZoomLevel <= 100) {
        return defaultZoomLevel;
    }

    return mTab->mStatusBar->mSlider->value();
}

FMWindowFactory *MainWindow::getFactory()
{
    return MainWindowFactory::getInstance();
}

const QString MainWindow::getCurrentUri()
{
    return mTab->getCurrentUri();
}

bool MainWindow::currentViewSupportZoom()
{
    if (getCurrentPage()) {
        if (auto view = getCurrentPage()->getView()) {
            return view->supportZoom();
        }
    }
    return mTab->mStatusBar->mSlider->isEnabled();
}

