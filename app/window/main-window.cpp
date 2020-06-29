#include "main-window.h"


MainWindow::MainWindow(const QString &uri, QWidget *parent)
{

}

QSize MainWindow::sizeHint() const
{

}

int MainWindow::getCurrentSortColumn()
{

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

}

const QStringList MainWindow::getCurrentSelections()
{

}

const QStringList MainWindow::getCurrentAllFileUris()
{

}

FMWindowIface *MainWindow::create(const QString &uri)
{

}

FMWindowIface *MainWindow::create(const QStringList &uris)
{

}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{

}

FMWindowIface *MainWindow::createWithZoomLevel(const QString &uri, int zoomLevel)
{

}

const QList<std::shared_ptr<FileInfo> > MainWindow::getCurrentSelectionFileInfos()
{

}

FMWindowIface *MainWindow::createWithZoomLevel(const QStringList &uris, int zoomLevel)
{

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
