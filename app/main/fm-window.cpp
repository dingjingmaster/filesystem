#include "fm-window.h"

FMWindow::FMWindow(const QString &uri, QWidget *parent)
{

}

int FMWindow::getCurrentSortColumn()
{

}

const QString FMWindow::getCurrentUri()
{

}

FMWindowFactory *FMWindow::getFactory()
{

}

Qt::SortOrder FMWindow::getCurrentSortOrder()
{

}

FMWindowIface *FMWindow::create(const QString &uri)
{

}

FMWindowIface *FMWindow::create(const QStringList &uris)
{

}

const QList<std::shared_ptr<FileInfo>> FMWindow::getCurrentSelectionFileInfos()
{

}

void FMWindow::slotRefresh()
{

}

void FMWindow::slotClearRecord()
{

}

void FMWindow::slotSetShowHidden()
{

}

void FMWindow::slotAdvanceSearch()
{

}

void FMWindow::slotForceStopLoading()
{

}

void FMWindow::slotSetSortFolderFirst()
{

}

void FMWindow::slotEditUri(const QString &uri)
{

}

void FMWindow::slotSetUseDefaultNameSortOrder()
{

}

void FMWindow::slotEditUris(const QStringList &uris)
{

}

void FMWindow::slotAddNewTabs(const QStringList &uris)
{

}

void FMWindow::slotSetCurrentSortColumn(int sortColumn)
{

}

void FMWindow::slotBeginSwitchView(const QString &viewId)
{

}

void FMWindow::slotOnPreviewPageSwitch(const QString &uri)
{

}

void FMWindow::slotSetCurrentSortOrder(Qt::SortOrder order)
{

}

void FMWindow::slotSetCurrentSelectionUris(const QStringList &uris)
{

}

void FMWindow::slotFilterUpdate(int type_index, int time_index, int size_index)
{

}

void FMWindow::slotGoToUri(const QString &uri, bool addHistory, bool forceUpdate)
{

}

void FMWindow::slotSearchFilter(QString target_path, QString keyWord, bool search_file_name, bool search_content)
{

}

void FMWindow::keyPressEvent(QKeyEvent *e)
{

}

void FMWindow::resizeEvent(QResizeEvent *e)
{

}

QSize FMWindow::sizeHint() const {
    return QSize(800, 600);
}

const QString FMWindow::getCurrentPageViewType()
{

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

}

DirectoryViewContainer *FMWindow::getCurrentPage()
{

}

const QStringList FMWindow::getCurrentAllFileUris()
{

}
