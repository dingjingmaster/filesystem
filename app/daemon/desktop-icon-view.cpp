#include "desktop-icon-view.h"

#include <QStandardPaths>

void DesktopIconView::bindModel(FileItemModel *sourceModel, FileItemProxyFilterSortModel *proxyModel)
{
    Q_UNUSED(sourceModel) Q_UNUSED(proxyModel)
}

void DesktopIconView::zoomIn()
{

}

void DesktopIconView::zoomOut()
{

}

void DesktopIconView::refresh()
{

}

void DesktopIconView::closeView()
{

}

void DesktopIconView::invertSelections()
{

}

void DesktopIconView::stopLocationChange()
{

}

void DesktopIconView::beginLocationChange()
{

}

void DesktopIconView::clearAllIndexWidgets()
{

}

DesktopIconView::ZoomLevel DesktopIconView::zoomLevel() const
{

}

void DesktopIconView::setSortType(int sortType)
{

}

void DesktopIconView::setSortOrder(int sortOrder)
{

}

void DesktopIconView::editUri(const QString &uri)
{

}

void DesktopIconView::saveAllItemPosistionInfos()
{

}

void DesktopIconView::resetAllItemPositionInfos()
{

}

DirectoryViewProxyIface *DesktopIconView::getProxy()
{
    return nullptr;
}

void DesktopIconView::editUris(const QStringList uris)
{

}

void DesktopIconView::setDirectoryUri(const QString &uri)
{

}

void DesktopIconView::setDefaultZoomLevel(DesktopIconView::ZoomLevel level)
{

}

void DesktopIconView::setCutFiles(const QStringList &uris)
{

}

void DesktopIconView::scrollToSelection(const QString &uri)
{

}

void DesktopIconView::setSelections(const QStringList &uris)
{

}

void DesktopIconView::saveItemPositionInfo(const QString &uri)
{

}

void DesktopIconView::resetItemPosistionInfo(const QString &uri)
{

}

void DesktopIconView::open(const QStringList &uris, bool newWindow)
{

}

void DesktopIconView::updateItemPosistions(const QString &uri)
{

}

bool DesktopIconView::isItemsOverlapped()
{

}

void DesktopIconView::dropEvent(QDropEvent *e)
{

}

void DesktopIconView::wheelEvent(QWheelEvent *e)
{

}

void DesktopIconView::keyPressEvent(QKeyEvent *e)
{

}

void DesktopIconView::resizeEvent(QResizeEvent *e)
{

}

void DesktopIconView::keyReleaseEvent(QKeyEvent *e)
{

}

void DesktopIconView::focusOutEvent(QFocusEvent *e)
{

}

void DesktopIconView::mousePressEvent(QMouseEvent *e)
{

}

void DesktopIconView::dragMoveEvent(QDragMoveEvent *e)
{

}

void DesktopIconView::mouseReleaseEvent(QMouseEvent *e)
{

}

void DesktopIconView::dragEnterEvent(QDragEnterEvent *e)
{

}

void DesktopIconView::mouseDoubleClickEvent(QMouseEvent *event)
{

}

void DesktopIconView::setProxy(DirectoryViewProxyIface *proxy)
{
    Q_UNUSED(proxy)
}

QRect DesktopIconView::visualRect(const QModelIndex &index) const
{

}

const QFont DesktopIconView::getViewItemFont(QStyleOptionViewItem *item)
{

}

DesktopIconView::DesktopIconView(QWidget *parent)
{

}

DesktopIconView::~DesktopIconView()
{

}

void DesktopIconView::initMenu()
{

}

int DesktopIconView::getSortType()
{

}

int DesktopIconView::getSortOrder()
{

}

void DesktopIconView::initShoutCut()
{

}

void DesktopIconView::initDoubleClick()
{

}

const QString DesktopIconView::viewId()
{
    return tr("Desktop Icon View");
}

const QString DesktopIconView::getDirectoryUri()
{
    return "file://" + QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

const QStringList DesktopIconView::getSelections()
{

}

const QStringList DesktopIconView::getAllFileUris()
{

}

bool DesktopIconView::eventFilter(QObject *obj, QEvent *e)
{

}
