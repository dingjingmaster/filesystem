#include "headerbar.h"

#include <QUrl>
#include <QTimer>
#include <QEvent>
#include <QProcess>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QApplication>
#include <kwindowsystem.h>
#include <window/main-window.h>
#include <syslog/clib_syslog.h>
#include <directory-view-container.h>
#include <qstyleoption.h>

static HeaderBarStyle *gInstance = nullptr;
static QString terminalCmd = nullptr;

HeaderBar::HeaderBar(MainWindow *parent) : QToolBar(parent)
{
    setAttribute(Qt::WA_AcceptTouchEvents);

    setMouseTracking(true);
    setStyle(HeaderBarStyle::getStyle());

    mWindow = parent;
    //disable default menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    setStyleSheet(".HeaderBar{"
                  "background-color: transparent;"
                  "border: 0px solid transparent;"
                  "margin: 4px 5px 4px 5px;"
                  "};");

    setMovable(false);

    auto a = addAction(QIcon::fromTheme("folder-new-symbolic"), tr("Create Folder"), [=]() {
        //use the same function
        mWindow->slotCreateFolderOperation();
    });
    auto createFolder = qobject_cast<QToolButton *>(widgetForAction(a));
    createFolder->setAutoRaise(false);
    createFolder->setFixedSize(QSize(40, 40));
    createFolder->setIconSize(QSize(16, 16));

    addSpacing(2);

    //find a terminal when init
    findDefaultTerminal();
    a = addAction(QIcon::fromTheme("terminal-app-symbolic"), tr("Open Terminal"), [=]() {
        //open the default terminal
        openDefaultTerminal();
    });
    auto openTerminal = qobject_cast<QToolButton *>(widgetForAction(a));
    openTerminal->setAutoRaise(false);
    openTerminal->setFixedSize(QSize(40, 40));
    openTerminal->setIconSize(QSize(16, 16));

    addSpacing(9);

    auto goBack = new HeadBarPushButton(this);
    mGoBack = goBack;
    goBack->setEnabled(false);
    goBack->setToolTip(tr("Go Back"));
    goBack->setFixedSize(QSize(36, 28));
    goBack->setIcon(QIcon::fromTheme("go-previous-symbolic"));
    addWidget(goBack);

    auto goForward = new HeadBarPushButton(this);
    mGoForward = goForward;
    goForward->setEnabled(false);
    goForward->setToolTip(tr("Go Forward"));
    goForward->setFixedSize(QSize(36, 28));
    goForward->setIcon(QIcon::fromTheme("go-next-symbolic"));
    addWidget(goForward);
    connect(goForward, &QPushButton::clicked, mWindow, [=]() {
        mWindow->getCurrentPage()->goForward();
    });

    addSpacing(9);

//    auto locationBar = new AdvancedLocationBar(this);
//    mLocationBar = locationBar;
//    addWidget(locationBar);

    connect(goBack, &QPushButton::clicked, mWindow, [=]() {
        mWindow->getCurrentPage()->goBack();
//        mLocationBar->clearSearchBox();
    });

//    connect(mLocationBar, &AdvancedLocationBar::refreshRequest, [=]() {
//        mWindow->updateTabPageTitle();
//    });
//    connect(mLocationBar, &AdvancedLocationBar::updateFileTypeFilter, [=](const int &index) {
//        mWindow->getCurrentPage()->setSortFilter(index);
//    });

//    connect(mLocationBar, &AdvancedLocationBar::updateWindowLocationRequest, this, &HeaderBar::updateLocationRequest);

    addSpacing(9);
    a = addAction(QIcon::fromTheme("edit-find-symbolic"), tr("Search"));
    connect(a, &QAction::triggered, this, &HeaderBar::searchButtonClicked);
    auto search = qobject_cast<QToolButton *>(widgetForAction(a));
    search->setAutoRaise(false);
    search->setFixedSize(QSize(40, 40));
    setIconSize(QSize(16, 16));
    mSearchButton = search;

    addSpacing(9);

    a = addAction(QIcon::fromTheme("view-grid-symbolic"), tr("View Type"));
    auto viewType = qobject_cast<QToolButton *>(widgetForAction(a));
    viewType->setAutoRaise(false);
    viewType->setFixedSize(QSize(57, 40));
    viewType->setIconSize(QSize(16, 16));
    viewType->setPopupMode(QToolButton::InstantPopup);

//    mViewTypeMenu = new ViewTypeMenu(viewType);
//    viewType->setMenu(mViewTypeMenu);

//    connect(mViewTypeMenu, &ViewTypeMenu::switchViewRequest, this, [=](const QString &id, const QIcon &icon, bool resetToZoomLevel) {
//        viewType->setText(id);
//        viewType->setIcon(icon);
//        this->viewTypeChangeRequest(id);
//        if (resetToZoomLevel) {
//            auto viewId = mWindow->getCurrentPage()->getView()->viewId();
//            auto factoryManger = DirectoryViewFactoryManager2::getInstance();
//            auto factory = factoryManger->getFactory(viewId);
//            int zoomLevelHint = factory->zoom_level_hint();
//            mWindow->getCurrentPage()->setZoomLevelRequest(zoomLevelHint);
//        }
//    });

//    connect(mViewTypeMenu, &ViewTypeMenu::updateZoomLevelHintRequest, this, &HeaderBar::updateZoomLevelHintRequest);

    addSpacing(2);

    a = addAction(QIcon::fromTheme("view-sort-ascending-symbolic"), tr("Sort Type"));
    auto sortType = qobject_cast<QToolButton *>(widgetForAction(a));
    sortType->setAutoRaise(false);
    sortType->setFixedSize(QSize(57, 40));
    sortType->setIconSize(QSize(16, 16));
    sortType->setPopupMode(QToolButton::InstantPopup);

//    mSortTypeMenu = new SortTypeMenu(this);
//    sortType->setMenu(mSortTypeMenu);

//    connect(mSortTypeMenu, &SortTypeMenu::switchSortTypeRequest, mWindow, &MainWindow::setCurrentSortColumn);
//    connect(mSortTypeMenu, &SortTypeMenu::switchSortOrderRequest, mWindow, [=](Qt::SortOrder order) {
//        if (order == Qt::AscendingOrder) {
//            sortType->setIcon(QIcon::fromTheme("view-sort-ascending-symbolic"));
//        } else {
//            sortType->setIcon(QIcon::fromTheme("view-sort-descending-symbolic"));
//        }
//        mWindow->setCurrentSortOrder(order);
//    });
//    connect(mSortTypeMenu, &QMenu::aboutToShow, mSortTypeMenu, [=]() {
//        mSortTypeMenu->setSortType(mWindow->getCurrentSortColumn());
//        mSortTypeMenu->setSortOrder(mWindow->getCurrentSortOrder());
//    });

    addSpacing(2);

    a = addAction(QIcon::fromTheme("open-menu-symbolic"), tr("Option"));
    auto popMenu = qobject_cast<QToolButton *>(widgetForAction(a));
    popMenu->setProperty("isOptionButton", true);
    popMenu->setAutoRaise(false);
    popMenu->setFixedSize(QSize(40, 40));
    popMenu->setIconSize(QSize(16, 16));
    popMenu->setPopupMode(QToolButton::InstantPopup);

//    mOperationMenu = new OperationMenu(mWindow, this);
//    popMenu->setMenu(mOperationMenu);

    for (auto action : actions()) {
        auto w = widgetForAction(action);
        w->setProperty("useIconHighlightEffect", true);
        w->setProperty("iconHighlightEffectMode", 1);
    }
}

void HeaderBar::findDefaultTerminal()
{
    GList *infos = g_app_info_get_all();
    GList *l = infos;
    while (l) {
        const char *cmd = g_app_info_get_executable(static_cast<GAppInfo*>(l->data));
        QString tmp = cmd;
        if (tmp.contains("terminal")) {
            terminalCmd = tmp;
            if (tmp == "mate-terminal") {
                break;
            }
        }
        l = l->next;
    }
    g_list_free_full(infos, g_object_unref);
}

void HeaderBar::openDefaultTerminal()
{
    //don't find any terminal
    if (terminalCmd == nullptr) {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setWindowTitle(tr("Operate Tips"));
        msgBox->setText(tr("Don't find any terminal, please install at least one terminal!"));
        msgBox->exec();
        return;
    }

    if (terminalCmd == "mate-terminal" || terminalCmd == "gnome-terminal") {
        auto url = mWindow->getCurrentUri();
        auto absPath = url.replace("file://", "");
        QProcess p;
        p.setProgram(terminalCmd);
        p.setArguments(QStringList()<<"--working-directory"<<absPath);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        p.startDetached();
#else
        p.startDetached(terminalCmd, QStringList()<<"--working-directory"<<absPath);
#endif
        p.waitForFinished(-1);
    } else {
        QUrl url = mWindow->getCurrentUri();
        auto directory = url.path().toUtf8().constData();
        gchar **argv = nullptr;
        g_shell_parse_argv (terminalCmd.toUtf8().constData(), nullptr, &argv, nullptr);
        GError *err = nullptr;
        g_spawn_async (directory,
                       argv,
                       nullptr,
                       G_SPAWN_SEARCH_PATH,
                       nullptr,
                       nullptr,
                       nullptr,
                       &err);
        if (err) {
            CT_SYSLOG(LOG_ERR, "");
            g_error_free(err);
            err = nullptr;
        }
        g_strfreev (argv);
    }
}

void HeaderBar::searchButtonClicked()
{
    mSearchMode = ! mSearchMode;
    setSearchMode(mSearchMode);
    Q_EMIT this->updateSearchRequest(mSearchMode);
}

void HeaderBar::setSearchMode(bool mode)
{
    mSearchButton->setCheckable(mode);
    mSearchButton->setChecked(mode);
    mSearchButton->setDown(mode);
//    mLocationBar->switchEditMode(mode);
}

void HeaderBar::closeSearch()
{
    mSearchMode = false;
    setSearchMode(false);
}

void HeaderBar::addSpacing(int pixel)
{
    for (int i = 0; i < pixel; i++) {
        addSeparator();
    }
}

void HeaderBar::mouseMoveEvent(QMouseEvent *e)
{
    QToolBar::mouseMoveEvent(e);
    QCursor c;
    c.setShape(Qt::ArrowCursor);
    this->topLevelWidget()->setCursor(c);
}

void HeaderBar::setLocation(const QString &uri)
{
//    mLocationBar->updateLocation(uri);
}

void HeaderBar::startEdit(bool bSearch)
{
    if (bSearch && mSearchMode) {
        return;
    }

    if (bSearch) {
        searchButtonClicked();
    } else {
        mSearchMode = false;
//        mLocationBar->startEdit();
//        mLocationBar->switchEditMode(false);
    }
}

void HeaderBar::finishEdit()
{
//    mLocationBar->finishEdit();
}

void HeaderBar::updateIcons()
{
//    qDebug()<<mWindow->getCurrentUri();
//    qDebug()<<mWindow->getCurrentSortColumn();
//    qDebug()<<mWindow->getCurrentSortOrder();
//    mViewTypeMenu->setCurrentDirectory(mWindow->getCurrentUri());
//    mViewTypeMenu->setCurrentView(mWindow->getCurrentPage()->getView()->viewId(), true);
//    mSortTypeMenu->switchSortTypeRequest(mWindow->getCurrentSortColumn());
//    mSortTypeMenu->switchSortOrderRequest(mWindow->getCurrentSortOrder());

    //go back & go forward
    mGoBack->setEnabled(mWindow->getCurrentPage()->canGoBack());
    mGoForward->setEnabled(mWindow->getCurrentPage()->canGoForward());

    mGoBack->setProperty("useIconHighlightEffect", true);
    mGoBack->setProperty("iconHighlightEffectMode", 1);
    mGoForward->setProperty("useIconHighlightEffect", true);
    mGoForward->setProperty("iconHighlightEffectMode", 1);

    //maximize & restore
    updateMaximizeState();
}

void HeaderBar::updateMaximizeState()
{
    //maximize & restore
    //do it in container
}

//HeaderBarToolButton
HeaderBarToolButton::HeaderBarToolButton(QWidget *parent) : QToolButton(parent)
{
    setAutoRaise(false);
    setIconSize(QSize(16, 16));
}

//HeadBarPushButton
HeadBarPushButton::HeadBarPushButton(QWidget *parent) : QPushButton(parent)
{
    setIconSize(QSize(16, 16));
}

//HeaderBarStyle
HeaderBarStyle::HeaderBarStyle()
{

}

HeaderBarStyle *HeaderBarStyle::getStyle()
{
    if (!gInstance) {
        gInstance = new HeaderBarStyle;
    }
    return gInstance;
}

int HeaderBarStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    if (qobject_cast<const HeaderBarContainer *>(widget))
        return 0;

    switch (metric) {
    case PM_ToolBarIconSize:
        return 16;
    case PM_ToolBarSeparatorExtent:
        return 1;
    case PM_ToolBarItemSpacing: {
        return 1;
    }
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

void HeaderBarStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    //This is a "lie". We want to use instant popup menu for tool button, and we aslo
    //want use popup menu style with this tool button, so we change the related flags
    //to draw in our expected.
    if (control == CC_ToolButton) {
        QStyleOptionToolButton button = *qstyleoption_cast<const QStyleOptionToolButton *>(option);
        if (button.features.testFlag(QStyleOptionToolButton::HasMenu)) {
            button.features = QStyleOptionToolButton::None;
            if (!widget->property("isOptionButton").toBool()) {
                button.features |= QStyleOptionToolButton::HasMenu;
                button.features |= QStyleOptionToolButton::MenuButtonPopup;
                button.subControls |= QStyle::SC_ToolButtonMenu;
            }
            return QProxyStyle::drawComplexControl(control, &button, painter, widget);
        }
    }
    return QProxyStyle::drawComplexControl(control, option, painter, widget);
}

void HeaderBarStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (element == PE_IndicatorToolBarSeparator) {
        return;
    }
    return QProxyStyle::drawPrimitive(element, option, painter, widget);
}

HeaderBarContainer::HeaderBarContainer(QWidget *parent) : QToolBar(parent)
{
    setStyle(HeaderBarStyle::getStyle());

    setContextMenuPolicy(Qt::CustomContextMenu);

    setStyleSheet(".HeaderBarContainer"
                  "{"
                  "background-color: transparent;"
                  "border: 0px solid transparent"
                  "}");

    setFixedHeight(50);
    setMovable(false);

    mLayout = new QHBoxLayout;
    mLayout->setSpacing(0);
    mLayout->setContentsMargins(0, 0, 0, 0);

    mInternalWidget = new QWidget(this);
    mInternalWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

bool HeaderBarContainer::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)
    auto window = qobject_cast<MainWindow *>(obj);
    if (window) {
        if (e->type() == QEvent::Resize) {
            if (window->isMaximized()) {
                mMaxOrRestore->setIcon(QIcon::fromTheme("window-restore-symbolic"));
                //mMaxOrRestore->setToolTip(tr("Restore"));
            } else {
                mMaxOrRestore->setIcon(QIcon::fromTheme("window-maximize-symbolic"));
                //mMaxOrRestore->setToolTip(tr("Maximize"));
            }
        }
        return false;
    } else {
        if (e->type() == QEvent::MouseMove) {
            //auto w = qobject_cast<QWidget *>(obj);
            QCursor c;
            c.setShape(Qt::ArrowCursor);
            //this->setCursor(c);
            //w->setCursor(c);
            this->topLevelWidget()->setCursor(c);
        }

    }

    return false;
}

void HeaderBarContainer::addHeaderBar(HeaderBar *headerBar)
{
    if (mHeaderBar) {
        return;
    }

    mHeaderBar = headerBar;

    headerBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mLayout->addWidget(headerBar);

    addWindowButtons();

    mInternalWidget->setLayout(mLayout);
    addWidget(mInternalWidget);

    mHeaderBar->mWindow->installEventFilter(this);
}

void HeaderBarContainer::addWindowButtons()
{
    //mWindow_buttons = new QWidget(this);
    auto layout = new QHBoxLayout;

    layout->setContentsMargins(0, 0, 4, 0);
    layout->setSpacing(4);

    //minimize, maximize and close
    auto minimize = new QToolButton(mInternalWidget);
    minimize->setIcon(QIcon::fromTheme("window-minimize-symbolic"));
    minimize->setToolTip(tr("Minimize"));
    minimize->setAutoRaise(false);
    minimize->setFixedSize(QSize(40, 40));
    minimize->setIconSize(QSize(16, 16));
    connect(minimize, &QToolButton::clicked, this, [=]() {
        KWindowSystem::minimizeWindow(mHeaderBar->mWindow->winId());
        //mHeaderBar->mWindow->showMinimized();
    });

    //window-maximize-symbolic
    //window-restore-symbolic
    auto maximizeAndRestore = new QToolButton(mInternalWidget);
    maximizeAndRestore->setToolTip(tr("Maximize/Restore"));
    maximizeAndRestore->setIcon(QIcon::fromTheme("window-maximize-symbolic"));
    maximizeAndRestore->setAutoRaise(false);
    maximizeAndRestore->setFixedSize(QSize(40, 40));
    maximizeAndRestore->setIconSize(QSize(16, 16));
    connect(maximizeAndRestore, &QToolButton::clicked, this, [=]() {
        mHeaderBar->mWindow->slotMaximizeOrRestore();

        bool maximized = mHeaderBar->mWindow->isMaximized();
        if (maximized) {
            maximizeAndRestore->setIcon(QIcon::fromTheme("window-restore-symbolic"));
            //maximizeAndRestore->setToolTip(tr("Restore"));
        } else {
            maximizeAndRestore->setIcon(QIcon::fromTheme("window-maximize-symbolic"));
            //maximizeAndRestore->setToolTip(tr("Maximize"));
        }
    });
    mMaxOrRestore = maximizeAndRestore;

    auto close = new QToolButton(mInternalWidget);
    close->setIcon(QIcon::fromTheme("window-close-symbolic"));
    close->setToolTip(tr("Close"));
    close->setAutoRaise(false);
    close->setFixedSize(QSize(40, 40));
    close->setIconSize(QSize(16, 16));
    connect(close, &QToolButton::clicked, this, [=]() {
        mHeaderBar->mWindow->close();
    });

    connect(qApp, &QApplication::paletteChanged, close, [=](){
        QTimer::singleShot(100, this, [=](){
            auto palette = qApp->palette();
            palette.setColor(QPalette::Highlight, QColor("#E54A50"));
            close->setPalette(palette);
        });
    });
    auto palette = qApp->palette();
    palette.setColor(QPalette::Highlight, QColor("#E54A50"));
    close->setPalette(palette);

    layout->addWidget(minimize);
    layout->addWidget(maximizeAndRestore);
    layout->addWidget(close);

    mLayout->addLayout(layout);

    minimize->setMouseTracking(true);
    minimize->installEventFilter(this);
    maximizeAndRestore->setMouseTracking(true);
    maximizeAndRestore->installEventFilter(this);
    close->setMouseTracking(true);
    close->installEventFilter(this);

    for (int i = 0; i < 3; i++) {
        auto w = layout->itemAt(i)->widget();
        w->setProperty("useIconHighlightEffect", true);
        w->setProperty("iconHighlightEffectMode", 1);
    }
}
