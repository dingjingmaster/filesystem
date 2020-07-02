#include "list-view.h"

#include "global-settings.h"
#include "list-view-style.h"

#include <QHeaderView>

#include <QVBoxLayout>
#include <QMouseEvent>

#include <QScrollBar>

#include <QWheelEvent>
#include <QDropEvent>
#include <QTimer>

#include <model/file-item-model.h>
#include <delegate/list-view-delegate.h>
#include <model/fileitem-proxy-filter-sort-model.h>

ListView::ListView(QWidget *parent) : QTreeView(parent)
{
    setStyle(ListViewStyle::getStyle());

    setAlternatingRowColors(true);
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);

    setItemDelegate(new ListViewDelegate(this));

    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setSectionsMovable(true);
    header()->setStretchLastSection(true);

    setExpandsOnDoubleClick(false);
    setSortingEnabled(true);

    setEditTriggers(QTreeView::NoEditTriggers);
    setDragEnabled(true);
    setDragDropMode(QTreeView::DragDrop);
    setSelectionMode(QTreeView::ExtendedSelection);

     mRenameTimer = new QTimer(this);
     mRenameTimer->setInterval(3000);
     mEditValid = false;
}

void ListView::scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint hint)
{
    // NOTE:
    // scrollTo() is confilcted with updateGeometry(), where it will
    // leave a space for view. So I override this method. However,
    // the fast keyboard locating of default tree view will be disabled
    // due to the function is overrided, too.

    //QTreeView::scrollTo(index, hint);
    //updateGeometries();
}

const QString ListView::viewId() {
    return tr("List View");
}

void ListView::bindModel(FileItemModel *sourceModel, FileItemProxyFilterSortModel *proxyModel)
{
    if (!sourceModel || !proxyModel)
        return;
     mModel = sourceModel;
     mProxyModel = proxyModel;
     mProxyModel->setSourceModel(mModel);
    setModel(proxyModel);
    //adjust columns layout.
    adjustColumnsSize();

    connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &selection, const QItemSelection &deselection) {
        auto currentSelections = selection.indexes();

        for (auto index : deselection.indexes()) {
            this->setIndexWidget(index, nullptr);
        }

        //rename trigger
        if (!currentSelections.isEmpty()) {
            int first_index_row = currentSelections.first().row();
            bool all_index_in_same_row = true;
            for (auto index : currentSelections) {
                if (first_index_row != index.row()) {
                    all_index_in_same_row = false;
                    break;
                }
            }
            if (all_index_in_same_row) {
                if( mLastIndex.row() != currentSelections.first().row())
                {
                     mEditValid = false;
                }
                 mLastIndex = currentSelections.first();
            }
        } else {
             mLastIndex = QModelIndex();
             mEditValid = false;
        }
    });
}

void ListView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        if (this->state() == QTreeView::EditingState) {
            if (indexWidget(indexAt(e->pos())))
                return;
        }
        Q_EMIT customContextMenuRequested(e->pos());
        return;
    }

     mEditValid = true;
    QTreeView::mousePressEvent(e);

    if (indexAt(e->pos()).column() != 0) {
        this->setState(QAbstractItemView::DragSelectingState);
    }

    //if click left button at blank space, it should select nothing
    if(e->button() == Qt::LeftButton && (!indexAt(e->pos()).isValid()) )
    {
        this->clearSelection();
        //this->clearFocus();
        return;
    }

    // mRenameTimer
    if(! mRenameTimer->isActive())
    {
         mRenameTimer->start();
         mEditValid = false;
    }
    else
    {
        //if remain time is between[0.75, 3000],then trigger rename event;
        //to make sure only click one row
        bool all_index_in_same_row = true;
        if (!this->selectedIndexes().isEmpty()) {
            int first_index_row = this->selectedIndexes().first().row();
            for (auto index : this->selectedIndexes()) {
                if (first_index_row != index.row()) {
                    all_index_in_same_row = false;
                    break;
                }
            }
        }
        //qDebug()<< mRenameTimer->remainingTime()<< mEditValid<<all_index_in_same_row;
        if( mRenameTimer->remainingTime()>=0 &&  mRenameTimer->remainingTime() <= 2250
                && indexAt(e->pos()) ==  mLastIndex &&  mLastIndex.isValid() &&  mEditValid == true && all_index_in_same_row)
        {
            slotRename();
        } else
        {
             mEditValid = false;
        }
    }


}

void ListView::mouseReleaseEvent(QMouseEvent *e)
{
    QTreeView::mouseReleaseEvent(e);

    if (e->button() != Qt::LeftButton) {
        return;
    }
}

void ListView::mouseDoubleClickEvent(QMouseEvent *event)
{
     mEditValid = false;

    QTreeView::mouseDoubleClickEvent(event);
}

void ListView::dragEnterEvent(QDragEnterEvent *e)
{
     mEditValid = false;
    QTreeView::dragEnterEvent(e);
}

void ListView::dropEvent(QDropEvent *e)
{
    if (e->source() == this) {
        // only handle the drop event on item.
        switch (dropIndicatorPosition()) {
        case QAbstractItemView::DropIndicatorPosition::OnItem: {
            break;
        }
        default:
            return;
        }
    }
    QTreeView::dropEvent(e);
}

void ListView::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);
    if (mLastSize != size()) {
        mLastSize = size();
        adjustColumnsSize();
    }
}

void ListView::updateGeometries()
{
    //add return to fix list view stuck before qt 5.12
    QTreeView::updateGeometries();
#if (QT_VERSION < QT_VERSION_CHECK(5, 12, 0))
    return;
#endif
    if (!model())
        return;

    if (model()->columnCount() == 0 || model()->rowCount() == 0)
        return;

    QStyleOptionViewItem opt = viewOptions();
    int height = itemDelegate()->sizeHint(opt, QModelIndex()).height();
    setViewportMargins(0, header()->height(), 0, height);
    verticalScrollBar()->setMaximum(verticalScrollBar()->maximum() + 1);
}

void ListView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        zoomLevelChangedRequest(e->delta() > 0);
        return;
    }
    QTreeView::wheelEvent(e);
}

void ListView::slotRename()
{
    QTimer::singleShot(300,  mRenameTimer, [&]() {
        if( mEditValid) {
             mRenameTimer->stop();
            setIndexWidget(mLastIndex, nullptr);
            edit(mLastIndex);
             mEditValid = false;
        }
    });

}

void ListView::setProxy(DirectoryViewProxyIface *proxy)
{

}

void ListView::resort()
{
     mProxyModel->sort(getSortType(), Qt::SortOrder(getSortOrder()));
}

void ListView::reportViewDirectoryChanged()
{
    Q_EMIT  mProxy->viewDirectoryChanged();
}

void ListView::adjustColumnsSize()
{
    if (!model())
        return;

    if (model()->columnCount() == 0)
        return;

    resizeColumnToContents(0);

    int rightPartsSize = 0;
    for (int column = 1; column < model()->columnCount(); column++) {
        resizeColumnToContents(column);
        int columnSize = header()->sectionSize(column);
        rightPartsSize += columnSize;
    }

    if (this->width() - rightPartsSize < BOTTOM_STATUS_MARGIN)
        return;

    //resizeColumnToContents(0);
    header()->resizeSection(0, this->viewport()->width() - rightPartsSize);
}

DirectoryViewProxyIface *ListView::getProxy()
{
    return  mProxy;
}

const QString ListView::getDirectoryUri()
{
    if (!mModel)
        return nullptr;
    return  mModel->getRootUri();
}

void ListView::setDirectoryUri(const QString &uri)
{
     mCurrentUri = uri;
}

const QStringList ListView::getSelections()
{
    QStringList uris;
    QModelIndexList selections = selectedIndexes();
    for (auto index : selections) {
        if (index.column() == 0)
            uris<<index.data(FileItemModel::UriRole).toString();
    }
    return uris;
}

void ListView::setSelections(const QStringList &uris)
{
    clearSelection();
    for (auto uri: uris) {
        const QModelIndex index = mProxyModel->indexFromUri(uri);
        if (index.isValid()) {
            auto flags = QItemSelectionModel::Select|QItemSelectionModel::Rows;
            selectionModel()->select(index, flags);
        }
    }
}

const QStringList ListView::getAllFileUris()
{
    return  mProxyModel->getAllFileUris();
}

void ListView::open(const QStringList &uris, bool newWindow)
{
    return;
}

void ListView::beginLocationChange()
{
     mEditValid = false;
    //setModel(nullptr);
     mModel->setRootUri( mCurrentUri);
}

void ListView::stopLocationChange()
{
     mModel->slotCancelFindChildren();
}

void ListView::closeView()
{
    this->deleteLater();
}

void ListView::invertSelections()
{
    QItemSelectionModel *selectionModel = this->selectionModel();
    const QItemSelection currentSelection = selectionModel->selection();
    this->selectAll();
    selectionModel->select(currentSelection, QItemSelectionModel::Deselect);
}

void ListView::scrollToSelection(const QString &uri)
{
    auto index =  mProxyModel->indexFromUri(uri);
    QTreeView::scrollTo(index);
}

void ListView::setCutFiles(const QStringList &uris)
{
    return;
}

int ListView::getSortType()
{
    int type =  mProxyModel->sortColumn();
    return type<0? 0: type;
}

void ListView::setSortType(int sortType)
{
     mProxyModel->sort(sortType, Qt::SortOrder(getSortOrder()));
}

int ListView::getSortOrder()
{
    return  mProxyModel->sortOrder();
}

void ListView::setSortOrder(int sortOrder)
{
     mProxyModel->sort(getSortType(), Qt::SortOrder(sortOrder));
}

void ListView::editUri(const QString &uri)
{
    setIndexWidget( mProxyModel->indexFromUri(uri), nullptr);
    edit( mProxyModel->indexFromUri(uri));
}

void ListView::editUris(const QStringList uris)
{
    //FIXME:
    //implement batch rename.
}

//List View 2
ListView2::ListView2(QWidget *parent) : DirectoryViewWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
     mView = new ListView(this);

    int defaultZoomLevel = GlobalSettings::getInstance()->getValue(DEFAULT_VIEW_ZOOM_LEVEL).toInt();
    if (defaultZoomLevel >= minimumZoomLevel() && defaultZoomLevel <= maximumZoomLevel())
         mZoomLevel = defaultZoomLevel;

    connect( mView, &ListView::zoomLevelChangedRequest, this, &ListView2::zoomRequest);
    layout->addWidget( mView);

    setLayout(layout);
}

ListView2::~ListView2()
{
     mModel->setPositiveResponse(true);
}

const QString ListView2::viewId()
{
    return "List View";
}

const QString ListView2::getDirectoryUri()
{
    return  mView->getDirectoryUri();
}

const QStringList ListView2::getSelections()
{
    return  mView->getSelections();
}

const QStringList ListView2::getAllFileUris()
{
    return  mView->getAllFileUris();
}

int ListView2::getSortType()
{
    return  mView->getSortType();
}

Qt::SortOrder ListView2::getSortOrder()
{
    return Qt::SortOrder( mView->getSortOrder());
}

int ListView2::currentZoomLevel()
{
    return  mZoomLevel;
}

int ListView2::minimumZoomLevel()
{
    return 0;
}

int ListView2::maximumZoomLevel()
{
    return 20;
}

bool ListView2::supportZoom()
{
    return true;
}

void ListView2::bindModel(FileItemModel *model, FileItemProxyFilterSortModel *proxyModel)
{
    disconnect( mModel);
    disconnect( mProxyModel);
     mModel = model;
     mProxyModel = proxyModel;

     mModel->setPositiveResponse(false);

     mView->bindModel(model, proxyModel);
    connect(model, &FileItemModel::findChildrenFinished, this, &DirectoryViewWidget::viewDirectoryChanged);
    connect(mModel, &FileItemModel::updated,  mView, &ListView::resort);

    connect(mView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &DirectoryViewWidget::viewSelectionChanged);

    connect(mView, &ListView::doubleClicked, this, [=](const QModelIndex &index) {
        Q_EMIT this->viewDoubleClicked(index.data(Qt::UserRole).toString());
    });


    connect(mView, &ListView::customContextMenuRequested, this, [=](const QPoint &pos) {
        if (! mView->indexAt(pos).isValid())
        {
             mView->clearSelection();
            // mView->clearFocus();
        }

        auto index =  mView->indexAt(pos);
        auto selectedIndexes =  mView->selectionModel()->selection().indexes();
        auto visualRect =  mView->visualRect(index);
        auto sizeHint =  mView->itemDelegate()->sizeHint( mView->viewOptions(), index);
        auto validRect = QRect(visualRect.topLeft(), sizeHint);
        if (!selectedIndexes.contains(index)) {
            if (!validRect.contains(pos)) {
                 mView->clearSelection();
                // mView->clearFocus();
            } else {
                auto flags = QItemSelectionModel::Select|QItemSelectionModel::Rows;
                 mView->clearSelection();
                // mView->clearFocus();
                 mView->selectionModel()->select( mView->indexAt(pos), flags);
            }
        }

        //NOTE: we have to ensure that we have cleared the
        //selection if menu request at blank pos.
        QTimer::singleShot(1, [=]() {
            Q_EMIT this->menuRequest(QCursor::pos());
        });
    });

    connect( mProxyModel, &FileItemProxyFilterSortModel::layoutChanged, this, [=]() {
        Q_EMIT this->sortOrderChanged(Qt::SortOrder(getSortOrder()));
    });
    connect( mProxyModel, &FileItemProxyFilterSortModel::layoutChanged, this, [=]() {
        Q_EMIT this->sortTypeChanged(getSortType());
    });

    connect( mModel, &FileItemModel::findChildrenFinished, this, [=]() {
        //delay a while for proxy model sorting.
        QTimer::singleShot(100, this, [=]() {
            // mView->setModel( mProxyModel);
            //adjust columns layout.
             mView->adjustColumnsSize();
        });
    });
}

void ListView2::setDirectoryUri(const QString &uri)
{
    mView->setDirectoryUri(uri);
}

void ListView2::beginLocationChange()
{
    mView->beginLocationChange();
}

void ListView2::stopLocationChange()
{
    mView->stopLocationChange();
}

void ListView2::closeDirectoryView()
{
    mView->closeView();
}

void ListView2::setSelections(const QStringList &uris)
{
    mView->setSelections(uris);
}

void ListView2::invertSelections()
{
    mView->invertSelections();
}

void ListView2::scrollToSelection(const QString &uri)
{
    mView->scrollToSelection(uri);
}

void ListView2::setCutFiles(const QStringList &uris)
{
    mView->setCutFiles(uris);
}

void ListView2::setSortType(int sortType)
{
    mView->setSortType(sortType);
}

void ListView2::setSortOrder(int sortOrder)
{
    mView->setSortOrder(sortOrder);
}

void ListView2::editUri(const QString &uri)
{
    mView->editUri(uri);
}

void ListView2::editUris(const QStringList uris)
{
    mView->editUris(uris);
}

void ListView2::setCurrentZoomLevel(int zoomLevel)
{
    int base = 16;
    int adjusted = base + zoomLevel;
     mView->setIconSize(QSize(adjusted, adjusted));
     mZoomLevel = zoomLevel;
}

void ListView2::clearIndexWidget()
{
    for (auto index :  mView->selectedIndexes()) {
         mView->setIndexWidget(index, nullptr);
    }
}
