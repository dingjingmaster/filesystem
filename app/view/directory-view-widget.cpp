#include "directory-view-widget.h"

#include <QApplication>

DirectoryViewWidget::DirectoryViewWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    connect(qApp, &QApplication::paletteChanged, this, [=]() {
        this->repaintView();
    });
}

DirectoryViewWidget::~DirectoryViewWidget ()
{

}

const QString DirectoryViewWidget::viewId()
{
    return "Directory View";
}

const QString DirectoryViewWidget::getDirectoryUri()
{
    return nullptr;
}

const QStringList DirectoryViewWidget::getSelections()
{
    return QStringList ();
}

const QStringList DirectoryViewWidget::getAllFileUris()
{
    return QStringList ();
}

int DirectoryViewWidget::currentZoomLevel()
{
    return -1;
}

int DirectoryViewWidget::minimumZoomLevel()
{
    return -1;
}

int DirectoryViewWidget::maximumZoomLevel()
{
    return -1;
}

void DirectoryViewWidget::repaintView()
{

}

void DirectoryViewWidget::invertSelections()
{

}

void DirectoryViewWidget::clearIndexWidget()
{

}

void DirectoryViewWidget::stopLocationChange()
{

}

void DirectoryViewWidget::closeDirectoryView()
{

}

void DirectoryViewWidget::beginLocationChange()
{

}

void DirectoryViewWidget::setSortType(int sortType)
{

}

void DirectoryViewWidget::setSortOrder(int sortOrder)
{

}

void DirectoryViewWidget::editUri(const QString &uri)
{

}

void DirectoryViewWidget::editUris(const QStringList uris)
{

}

void DirectoryViewWidget::setCurrentZoomLevel(int zoomLevel)
{

}

void DirectoryViewWidget::setDirectoryUri(const QString &uri)
{

}

void DirectoryViewWidget::setCutFiles(const QStringList &uris)
{

}

void DirectoryViewWidget::scrollToSelection(const QString &uri)
{

}

void DirectoryViewWidget::setSelections(const QStringList &uris)
{

}

void DirectoryViewWidget::bindModel(FileItemModel *model, FileItemProxyFilterSortModel *proxyModel)
{

}

bool DirectoryViewWidget::supportZoom()
{
    return false;
}

Qt::SortOrder DirectoryViewWidget::getSortOrder()
{
    return Qt::AscendingOrder;
}

int DirectoryViewWidget::getSortType()
{
    return 0;
}
