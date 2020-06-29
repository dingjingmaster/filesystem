#include "fm-window-iface.h"

FMWindowFactory *FMWindowIface::getFactory()
{
    return nullptr;
}

const QString FMWindowIface::getLastNonSearchUri()
{
    return nullptr;
}

const QStringList FMWindowIface::getCurrentSelections()
{
    return QStringList();
}

const QStringList FMWindowIface::getCurrentAllFileUris()
{
    return QStringList();
}

const QList<std::shared_ptr<FileInfo> > FMWindowIface::getCurrentSelectionFileInfos()
{
    return QList<std::shared_ptr<FileInfo>>();
}

void FMWindowIface::slotRefresh() {}

void FMWindowIface::slotClearRecord() {}

void FMWindowIface::slotAdvanceSearch() {}

void FMWindowIface::slotSetShowHidden() {}

void FMWindowIface::slotForceStopLoading() {}

void FMWindowIface::slotSetSortFolderFirst() {}

void FMWindowIface::slotEditUri(const QString &uri) {}

void FMWindowIface::slotSetUseDefaultNameSortOrder() {}

void FMWindowIface::slotEditUris(const QStringList &uris) {}

void FMWindowIface::slotAddNewTabs(const QStringList &uris) {}

void FMWindowIface::slotSetCurrentSortColumn(int sortColumn) {}

void FMWindowIface::slotBeginSwitchView(const QString &viewId) {}

void FMWindowIface::slotSetCurrentViewZoomLevel(int zoomLevel) {}

void FMWindowIface::slotOnPreviewPageSwitch(const QString &uri) {}

void FMWindowIface::slotSetCurrentSortOrder(Qt::SortOrder order) {}

void FMWindowIface::slotSetCurrentSelectionUris(const QStringList &uris) {}

void FMWindowIface::slotFilterUpdate(int type_index, int time_index, int size_index) {}

void FMWindowIface::slotGoToUri(const QString &uri, bool addHistory, bool forceUpdate) {}

void FMWindowIface::slotSearchFilter(QString target_path, QString keyWord, bool search_file_name, bool search_content) {}

DirectoryViewContainer *FMWindowIface::getCurrentPage()
{
    return nullptr;
}

Qt::SortOrder FMWindowIface::getCurrentSortOrder()
{
    return Qt::AscendingOrder;
}

int FMWindowIface::getCurrentSortColumn()
{
    return 0;
}

bool FMWindowIface::getWindowShowHidden()
{
    return false;
}

bool FMWindowIface::getWindowUseDefaultNameSortOrder()
{
    return false;
}

bool FMWindowIface::getWindowSortFolderFirst() {
    return false;
}

const QString FMWindowIface::getCurrentPageViewType() {
    return nullptr;
}

int FMWindowIface::currentViewZoomLevel() {
    return -1;
}

bool FMWindowIface::currentViewSupportZoom() {
    return false;
}
