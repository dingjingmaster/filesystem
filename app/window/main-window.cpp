#include "main-window.h"

#include <QScreen>
#include <QMouseEvent>
#include <QApplication>
#include <clib_syslog.h>
#include <global-settings.h>

static MainWindow* gLastResizeWindow = nullptr;

MainWindow::MainWindow(const QString &uri, QWidget *parent)
{
    CT_SYSLOG(LOG_DEBUG, "MainWindow construct ...");
    installEventFilter(this);

    setWindowIcon(QIcon::fromTheme("system-file-manager"));
    setWindowTitle(tr("File Manager"));

    slotCheckSettings();

//    mEffect = new BorderShadowEffect(this);
//    mEffect->setPadding(4);
//    mEffect->setBorderRadius(6);
//    mEffect->setBlurRadius(4);
    //setGraphicsEffect(m_effect);

    setAnimated(false);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //fix double window base buttons issue, not effect MinMax button hints
    auto flags = windowFlags() &~Qt::WindowMinMaxButtonsHint;
    setWindowFlags(flags |Qt::FramelessWindowHint);
    //setWindowFlags(windowFlags()|Qt::FramelessWindowHint);
    setContentsMargins(4, 4, 4, 4);

    //bind resize handler
//    auto handler = new QWidgetResizeHandler(this);
//    handler->setMovingEnabled(false);
//    mResizeHandler = handler;

    //disable style window manager
//    setProperty("useStyleWindowManager", false);

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
//    return mTab->getSortType();
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
//    return mTab->currentPage();
}

const QStringList MainWindow::getCurrentSelections()
{
//    return mTab->getCurrentSelections();
}

const QStringList MainWindow::getCurrentAllFileUris()
{
//    return mTab->getAllFileUris();
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
        if (currentViewSupportZoom())
            window->slotSetCurrentViewZoomLevel(this->currentViewZoomLevel());
        return window;
    }
    auto uri = uris.first();
    auto l = uris;
    l.removeAt(0);
    auto window = new MainWindow(uri);
    if (currentViewSupportZoom()) {
        window->slotSetCurrentViewZoomLevel(this->currentViewZoomLevel());
    }

//    window->addNewTabs(l);

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
//            settings->setValue(DEFAULT_SIDEBAR_WIDTH, mSideBar->width());
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
//        auto info = FileInfo::fromUri(uri);
//        infos << info;
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

//    window->addNewTabs(l);

    return window;
}

void MainWindow::slotRefresh()
{

}

void MainWindow::slotCleanTrash()
{

}

void MainWindow::slotClearRecord()
{

}

void MainWindow::slotSetShortCuts()
{

}

void MainWindow::slotAdvanceSearch()
{

}

void MainWindow::slotSetShowHidden()
{

}

void MainWindow::slotCheckSettings()
{

}

void MainWindow::slotUpdateHeaderBar()
{

}

void MainWindow::slotForceStopLoading()
{

}

void MainWindow::slotRecoverFromTrash()
{

}

void MainWindow::slotMaximizeOrRestore()
{

}

void MainWindow::slotSetSortFolderFirst()
{

}

void MainWindow::slotUpdateTabPageTitle()
{

}

void MainWindow::slotCreateFolderOperation()
{

}

void MainWindow::slotEditUri(const QString &uri)
{

}

void MainWindow::slotSetUseDefaultNameSortOrder()
{

}

void MainWindow::slotSetLabelNameFilter(QString name)
{

}

void MainWindow::slotEditUris(const QStringList &uris)
{

}

void MainWindow::slotAddNewTabs(const QStringList &uris)
{

}

void MainWindow::slotSetCurrentSortColumn(int sortColumn)
{

}

void MainWindow::slotSetCurrentViewZoomLevel(int zoomLevel)
{

}

void MainWindow::slotBeginSwitchView(const QString &viewId)
{

}

void MainWindow::slotSyncControlsLocation(const QString &uri)
{

}

void MainWindow::slotSetCurrentSortOrder(Qt::SortOrder order)
{

}

void MainWindow::slotSetCurrentSelectionUris(const QStringList &uris)
{

}

void MainWindow::slotFilterUpdate(int type_index, int time_index, int size_index)
{

}

void MainWindow::slotGoToUri(const QString &uri, bool addHistory, bool force)
{

}

void MainWindow::slotSearchFilter(QString target_path, QString keyWord, bool search_file_name, bool search_content)
{

}

void MainWindow::validBorder()
{

}

QRect MainWindow::sideBarRect()
{

}

void MainWindow::initAdvancePage()
{

}

void MainWindow::initUI(const QString &uri)
{

}

void MainWindow::paintEvent(QPaintEvent *e)
{

}

void MainWindow::keyPressEvent(QKeyEvent *e)
{

}

void MainWindow::resizeEvent(QResizeEvent *e)
{

}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{

}

void MainWindow::mousePressEvent(QMouseEvent *e)
{

}

void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{

}


bool MainWindow::getWindowSortFolderFirst()
{
    return mFolderFirst;
}

Qt::SortOrder MainWindow::getCurrentSortOrder()
{

}

int MainWindow::currentViewZoomLevel()
{

}

FMWindowFactory *MainWindow::getFactory()
{

}

const QString MainWindow::getCurrentUri()
{

}

bool MainWindow::currentViewSupportZoom()
{

}
