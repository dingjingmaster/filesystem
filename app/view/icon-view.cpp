#include "icon-view.h"

#include "global-settings.h"
#include "icon-view-style.h"

#include <QMouseEvent>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QDragMoveEvent>
#include <QDropEvent>

#include <QPainter>
#include <QPaintEvent>

#include <QApplication>

#include <QVBoxLayout>

#include <QHoverEvent>

#include <QScrollBar>

#include <QDebug>

#include "model/file-item.h"
#include <model/file-item-model.h>
#include <model/fileitem-proxy-filter-sort-model.h>

#include <delegate/icon-view-delegate.h>

IconView::IconView(QWidget *parent) : QListView(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    viewport()->setAttribute(Qt::WA_TranslucentBackground);

    setStyle(IconViewStyle::getStyle());
    IconViewDelegate *delegate = new IconViewDelegate(this);
    setItemDelegate(delegate);

    setSelectionMode(QListView::ExtendedSelection);
    setEditTriggers(QListView::NoEditTriggers);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Snap);
    //setWordWrap(true);

    setContextMenuPolicy(Qt::CustomContextMenu);

    setGridSize(QSize(115, 135));
    setIconSize(QSize(64, 64));


     mRenameTimer = new QTimer(this);
     mRenameTimer->setInterval(3000);
     mEditValid = false;
}

IconView::~IconView()
{

}

DirectoryViewProxyIface *IconView::getProxy()
{
    return  mProxy;
}

void IconView::setSelections(const QStringList &uris)
{
    clearSelection();
    for (auto uri: uris) {
        const QModelIndex index =  mSortFilterProxyModel->indexFromUri(uri);
        if (index.isValid()) {
            selectionModel()->select(index, QItemSelectionModel::Select);
        }
    }
}

const QStringList IconView::getSelections()
{
    QStringList uris;
    QModelIndexList selections = selectedIndexes();
    for (auto index : selections) {
        auto item =  mSortFilterProxyModel->itemFromIndex(index);
        uris << item->uri();
    }
    return uris;
}

void IconView::invertSelections()
{
    QItemSelectionModel *selectionModel = this->selectionModel();
    const QItemSelection currentSelection = selectionModel->selection();
    this->selectAll();
    selectionModel->select(currentSelection, QItemSelectionModel::Deselect);
}

void IconView::scrollToSelection(const QString &uri)
{
    auto index =  mSortFilterProxyModel->indexFromUri(uri);
    scrollTo(index);
}

//clipboard
void IconView::setCutFiles(const QStringList &uris)
{
    //let delegate and model know how to deal with cut files.
}

//location
//FIXME: implement location functions.
void IconView::setDirectoryUri(const QString &uri)
{
     mCurrentUri = uri;
}

const QString IconView::getDirectoryUri()
{
    return  mModel->getRootUri();
}

void IconView::beginLocationChange()
{
     mEditValid = false;
     mModel->setRootUri( mCurrentUri);
}

void IconView::stopLocationChange()
{
     mModel->slotCancelFindChildren();
}

//other
void IconView::open(const QStringList &uris, bool newWindow)
{

}

void IconView::closeView()
{
    this->deleteLater();
}

void IconView::dragEnterEvent(QDragEnterEvent *e)
{
     mEditValid = false;
    qDebug()<<"dragEnterEvent()";
    if (e->mimeData()->hasUrls()) {
        e->setDropAction(Qt::MoveAction);
        e->accept();
    }
}

void IconView::dragMoveEvent(QDragMoveEvent *e)
{
    qDebug()<<"dragMoveEvent()";
    auto index = indexAt(e->pos());
    if (index.isValid() && index !=  mLastIndex) {
        QHoverEvent he(QHoverEvent::HoverMove, e->posF(), e->posF());
        viewportEvent(&he);
    } else {
        QHoverEvent he(QHoverEvent::HoverLeave, e->posF(), e->posF());
        viewportEvent(&he);
    }
    if (this == e->source()) {
        return QListView::dragMoveEvent(e);
    }
    e->setDropAction(Qt::MoveAction);
    e->accept();
}

void IconView::dropEvent(QDropEvent *e)
{
     mLastIndex = QModelIndex();
    //m_edit_trigger_timer.stop();
    e->setDropAction(Qt::MoveAction);
    auto proxy_index = indexAt(e->pos());
    auto index =  mSortFilterProxyModel->mapToSource(proxy_index);
    qDebug()<<"dropEvent";
    if (e->source() == this) {
        if (indexAt(e->pos()).isValid()) {
             mModel->dropMimeData(e->mimeData(), Qt::MoveAction, 0, 0, index);
            return;
        }
        else {
            return;
        }
    }
     mModel->dropMimeData(e->mimeData(), Qt::MoveAction, 0, 0, index);
}

void IconView::mousePressEvent(QMouseEvent *e)
{
    qDebug()<<"moursePressEvent";
     mEditValid = true;
    QListView::mousePressEvent(e);

    if (e->button() != Qt::LeftButton) {
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
        if( mRenameTimer->remainingTime()>=0 &&  mRenameTimer->remainingTime() <= 2250
                && indexAt(e->pos()) ==  mLastIndex &&  mLastIndex.isValid() &&  mEditValid == true)
        {
            slotRename();
        } else
        {
             mEditValid = false;
        }
    }

}

void IconView::mouseReleaseEvent(QMouseEvent *e)
{
    QListView::mouseReleaseEvent(e);

    if (e->button() != Qt::LeftButton) {
        return;
    }
}

void IconView::mouseDoubleClickEvent(QMouseEvent *event)
{
     mEditValid = false;
    QListView::mouseDoubleClickEvent(event);
}

void IconView::paintEvent(QPaintEvent *e)
{
    QPainter p(this->viewport());
    p.fillRect(this->geometry(), this->palette().base());
    if ( mRepaintTimer.isActive()) {
         mRepaintTimer.stop();
        QTimer::singleShot(100, this, [this]() {
            this->repaint();
        });
    }
    QListView::paintEvent(e);
}

void IconView::resizeEvent(QResizeEvent *e)
{
    //FIXME: first resize is disfluency.
    //but I have to reset the index widget in view's resize.
    QListView::resizeEvent(e);
    setIndexWidget( mLastIndex, nullptr);
}

void IconView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->delta() > 0) {
            zoomLevelChangedRequest(true);
        } else {
            zoomLevelChangedRequest(false);
        }
        return;
    }

    QListView::wheelEvent(e);
    if (e->buttons() == Qt::LeftButton)
        this->viewport()->update();
}

void IconView::updateGeometries()
{
    QListView::updateGeometries();

    if (!model())
        return;

    if (model()->columnCount() == 0 || model()->rowCount() == 0)
        return;

    verticalScrollBar()->setMaximum(verticalScrollBar()->maximum() + BOTTOM_STATUS_MARGIN);
}

void IconView::slotRename()
{
    //delay edit action to avoid doubleClick or dragEvent
    qDebug()<<"slotRename"<< mEditValid;
    QTimer::singleShot(300,  mRenameTimer, [&]() {
        qDebug()<<"singleshot"<< mEditValid;
        if( mEditValid) {
             mRenameTimer->stop();
            setIndexWidget( mLastIndex, nullptr);
            edit( mLastIndex);
             mEditValid = false;
        }
    });

}

void IconView::bindModel(FileItemModel *sourceModel, FileItemProxyFilterSortModel *proxyModel)
{
     mModel = sourceModel;
     mSortFilterProxyModel = proxyModel;

    setModel( mSortFilterProxyModel);

    //edit trigger
    connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &selection, const QItemSelection &deselection) {
        auto currentSelections = selection.indexes();

        for (auto index : deselection.indexes()) {
            this->setIndexWidget(index, nullptr);
        }

        //Q_EMIT  mProxy->viewSelectionChanged();
        if (currentSelections.count() == 1) {
            if( mLastIndex != currentSelections.first()) {
                 mEditValid = false;
            }
             mLastIndex = currentSelections.first();
            //qDebug()<<"IconView::bindModel:"<<"selection changed: "<<"resetEditTriggerTimer";
            //this->resetEditTriggerTimer();
        } else {
             mLastIndex = QModelIndex();
             mEditValid = false;
        }


        qDebug()<<"selection changed2"<< mEditValid;
    });
}

void IconView::setProxy(DirectoryViewProxyIface *proxy)
{
    if (!proxy)
        return;

     mProxy = proxy;
    if (! mProxy) {
        return;
    }

    //connect( mModel, &FileItemModel::dataChanged, this, &IconView::clearIndexWidget);
    connect( mModel, &FileItemModel::updated, this, &IconView::resort);

    connect( mModel, &FileItemModel::findChildrenFinished,
            this, &IconView::reportViewDirectoryChanged);

    connect(this, &IconView::doubleClicked, [=](const QModelIndex &index) {
        qDebug()<<"double click"<<index.data(FileItemModel::UriRole);
        Q_EMIT  mProxy->viewDoubleClicked(index.data(FileItemModel::UriRole).toString());
    });



    //menu
//    connect(this, &IconView::customContextMenuRequested, [=](const QPoint &pos){
//        if (!indexAt(pos).isValid())
//            this->clearSelection();

//        //NOTE: we have to ensure that we have cleared the
//        //selection if menu request at blank pos.
//        QTimer::singleShot(1, [=](){
//            Q_EMIT this->getProxy()->menuRequest(QCursor::pos());
//        });
//    });
}

// NOTE: When icon view was resorted,
// index widget would deviated from its normal position by somehow.
// So, do not set any index widget when the resorting.
void IconView::resort()
{
    //fix uncompress selected file will cover file before it issue
    clearIndexWidget();
    if ( mLastIndex.isValid()) {
        this->setIndexWidget( mLastIndex, nullptr);
    }

    if ( mSortFilterProxyModel)
         mSortFilterProxyModel->sort(getSortType(), Qt::SortOrder(getSortOrder()));
}

void IconView::reportViewDirectoryChanged()
{
    if ( mProxy)
        Q_EMIT  mProxy->viewDirectoryChanged();
}

QRect IconView::visualRect(const QModelIndex &index) const
{
    auto rect = QListView::visualRect(index);
    rect.setX(rect.x() + 10);
    rect.setY(rect.y() + 15);
    auto size = itemDelegate()->sizeHint(QStyleOptionViewItem(), index);
    rect.setSize(size);
    return rect;
}

int IconView::getSortType()
{
    int type =  mSortFilterProxyModel->sortColumn();
    return type<0? 0: type;
}

void IconView::setSortType(int sortType)
{
     mSortFilterProxyModel->sort(sortType, Qt::SortOrder(getSortOrder()));
}

int IconView::getSortOrder()
{
    return  mSortFilterProxyModel->sortOrder();
}

void IconView::setSortOrder(int sortOrder)
{
     mSortFilterProxyModel->sort(getSortType(), Qt::SortOrder(sortOrder));
}

const QStringList IconView::getAllFileUris()
{
    return  mSortFilterProxyModel->getAllFileUris();
}

void IconView::editUri(const QString &uri)
{
    setIndexWidget( mSortFilterProxyModel->indexFromUri(uri), nullptr);
    edit( mSortFilterProxyModel->indexFromUri(uri));
}

void IconView::editUris(const QStringList uris)
{
    //FIXME:
    //implement batch rename.
}

void IconView::clearIndexWidget()
{
    for (int i = 0; i <  mSortFilterProxyModel->rowCount(); i++) {
        auto index =  mSortFilterProxyModel->index(i, 0);
        setIndexWidget(index, nullptr);
    }
}

//Icon View 2
IconView2::IconView2(QWidget *parent) : DirectoryViewWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
     mView = new IconView(this);

    int defaultZoomLevel = GlobalSettings::getInstance()->getValue(DEFAULT_VIEW_ZOOM_LEVEL).toInt();
    if (defaultZoomLevel >= minimumZoomLevel() && defaultZoomLevel <= maximumZoomLevel())
         mZoomLevel = defaultZoomLevel;

    connect( mView, &IconView::zoomLevelChangedRequest, this, &IconView2::zoomRequest);

    layout->addWidget( mView);

    setLayout(layout);
}

IconView2::~IconView2()
{

}

void IconView2::bindModel(FileItemModel *model, FileItemProxyFilterSortModel *proxyModel)
{
    disconnect(mModel);
    disconnect(mProxyModel);
     mModel = model;
     mProxyModel = proxyModel;

     mView->bindModel(model, proxyModel);
    connect(model, &FileItemModel::findChildrenFinished, this, &DirectoryViewWidget::viewDirectoryChanged);
    //connect( mModel, &FileItemModel::dataChanged,  mView, &IconView::clearIndexWidget);
    connect( mModel, &FileItemModel::updated,  mView, &IconView::resort);

    connect( mView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &DirectoryViewWidget::viewSelectionChanged);

    connect( mView, &IconView::doubleClicked, this, [=](const QModelIndex &index) {
        Q_EMIT this->viewDoubleClicked(index.data(Qt::UserRole).toString());
    });

    connect( mView, &IconView::customContextMenuRequested, this, [=](const QPoint &pos) {
        if (! mView->indexAt(pos).isValid())
             mView->clearSelection();

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
}

void IconView2::repaintView()
{
     mView->update();
     mView->viewport()->update();
}

void IconView2::setCurrentZoomLevel(int zoomLevel)
{
    if (zoomLevel <= maximumZoomLevel() && zoomLevel >= minimumZoomLevel()) {
         mZoomLevel = zoomLevel;
        //FIXME: implement zoom
        int base = 64 - 25; //50
        int adjusted = base + zoomLevel;
         mView->setIconSize(QSize(adjusted, adjusted));
         mView->setGridSize( mView->itemDelegate()->sizeHint(QStyleOptionViewItem(), QModelIndex()) + QSize(20, 20));
    }
}

void IconView2::clearIndexWidget()
{
    for (auto index :  mView->selectedIndexes()) {
         mView->setIndexWidget(index, nullptr);
    }
}


