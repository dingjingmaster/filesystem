#include "desktop-icon-view-delegate.h"
#include "desktop-icon-view.h"
#include "desktop-index-widget.h"

#include <syslog/clib_syslog.h>

#include <QAction>
#include <QProcess>
#include <gio/gio.h>
#include <QMimeData>
#include <QDropEvent>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QStandardPaths>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDesktopServices>
#include <global-settings.h>
#include <file-operation-utils.h>
#include <clipbord-utils.h>
#include <icon-view-style.h>
#include <file/file-info-job.h>
#include <file/file-info.h>
#include <file/file-launch-manager.h>
#include <file/file-meta-info.h>
#include <file/file-operation-manager.h>
#include <file/file-trash-operation.h>
#include <model/file-item-model.h>
#include <window/properties-window.h>

#define ITEM_POS_ATTRIBUTE "metadata::filesystem-item-position"

void DesktopIconView::bindModel(FileItemModel *sourceModel, FileItemProxyFilterSortModel *proxyModel)
{
    Q_UNUSED(sourceModel) Q_UNUSED(proxyModel)
}

void DesktopIconView::zoomIn()
{
    clearAllIndexWidgets();
    switch (zoomLevel()) {
    case Small:
        setDefaultZoomLevel(Normal);
        break;
    case Normal:
        setDefaultZoomLevel(Large);
        break;
    case Large:
        setDefaultZoomLevel(Huge);
        break;
    default:
        setDefaultZoomLevel(zoomLevel());
        break;
    }
}

void DesktopIconView::zoomOut()
{
    clearAllIndexWidgets();
    switch (zoomLevel()) {
    case Huge:
        setDefaultZoomLevel(Large);
        break;
    case Large:
        setDefaultZoomLevel(Normal);
        break;
    case Normal:
        setDefaultZoomLevel(Small);
        break;
    default:
        setDefaultZoomLevel(zoomLevel());
        break;
    }
}

void DesktopIconView::refresh()
{
    if (mRefreshTimer.isActive())
        return;

    if (!mModel)
        return;

    if (mIsRefreshing)
        return;
    mIsRefreshing = true;
    mModel->refresh();

    mRefreshTimer.start();
}

void DesktopIconView::closeView()
{
    deleteLater();
}

void DesktopIconView::invertSelections()
{
    QItemSelectionModel *selectionModel = this->selectionModel();
    const QItemSelection currentSelection = selectionModel->selection();
    this->selectAll();
    selectionModel->select(currentSelection, QItemSelectionModel::Deselect);
}

void DesktopIconView::stopLocationChange()
{

}

void DesktopIconView::beginLocationChange()
{

}

void DesktopIconView::clearAllIndexWidgets()
{
    if (!model())
        return;

    int row = 0;
    auto index = model()->index(row, 0);
    while (index.isValid()) {
        setIndexWidget(index, nullptr);
        row++;
        index = model()->index(row, 0);
    }
}

DesktopIconView::ZoomLevel DesktopIconView::zoomLevel() const
{
    if (mZoomLevel != Invalid)
        return mZoomLevel;

    auto metaInfo = FileMetaInfo::fromUri("computer:///");
    if (metaInfo) {
        auto i = metaInfo->getMetaInfoInt("peony-qt-desktop-zoom-level");
        return ZoomLevel(i);
    }

    GFile *computer = g_file_new_for_uri("computer:///");
    GFileInfo *info = g_file_query_info(computer,
                                        "metadata::peony-qt-desktop-zoom-level",
                                        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                        nullptr,
                                        nullptr);
    char* zoom_level = g_file_info_get_attribute_as_string(info, "metadata::peony-qt-desktop-zoom-level");
    if (!zoom_level) {
        g_object_unref(info);
        g_object_unref(computer);
        return Normal;
    }
    g_object_unref(info);
    g_object_unref(computer);
    QString zoomLevel = zoom_level;
    g_free(zoom_level);
    return ZoomLevel(zoomLevel.toInt()) == Invalid? Normal: ZoomLevel(QString(zoomLevel).toInt());
}

void DesktopIconView::setSortType(int sortType)
{
    mProxyModel->setSortType(sortType);
    mProxyModel->sort(1);
    mProxyModel->sort(0, mProxyModel->sortOrder());
}

void DesktopIconView::setSortOrder(int sortOrder)
{
    mProxyModel->sort(0, Qt::SortOrder(sortOrder));
}

void DesktopIconView::editUri(const QString &uri)
{
    clearAllIndexWidgets();
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
    QTimer::singleShot(100, this, [=]() {
#else
    QTimer::singleShot(100, [=]() {
#endif
        edit(mProxyModel->mapFromSource(mModel->indexFromUri(uri)));
    });
}

void DesktopIconView::saveAllItemPosistionInfos()
{
    CT_SYSLOG(LOG_DEBUG, "");
    for (int i = 0; i < mProxyModel->rowCount(); i++) {
        auto index = mProxyModel->index(i, 0);
        auto indexRect = QListView::visualRect(index);
        QStringList topLeft;
        topLeft<<QString::number(indexRect.top());
        topLeft<<QString::number(indexRect.left());

        auto metaInfo = FileMetaInfo::fromUri(index.data(Qt::UserRole).toString());
        if (metaInfo) {
            metaInfo->setMetaInfoStringList(ITEM_POS_ATTRIBUTE, topLeft);
        }
    }
}

void DesktopIconView::resetAllItemPositionInfos()
{
    for (int i = 0; i < mProxyModel->rowCount(); i++) {
        auto index = mProxyModel->index(i, 0);
        auto indexRect = QListView::visualRect(index);
        QStringList topLeft;
        topLeft<<QString::number(indexRect.top());
        topLeft<<QString::number(indexRect.left());
        auto metaInfo = FileMetaInfo::fromUri(index.data(Qt::UserRole).toString());
        if (metaInfo) {
            QStringList tmp;
            tmp<<"-1"<<"-1";
            metaInfo->setMetaInfoStringList(ITEM_POS_ATTRIBUTE, tmp);
        }
    }
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
    mZoomLevel = level;
    switch (level) {
    case Small:
        setIconSize(QSize(24, 24));
        setGridSize(QSize(64, 64));
        break;
    case Large:
        setIconSize(QSize(64, 64));
        setGridSize(QSize(115, 135));
        break;
    case Huge:
        setIconSize(QSize(96, 96));
        setGridSize(QSize(140, 170));
        break;
    default:
        mZoomLevel = Normal;
        setIconSize(QSize(48, 48));
        setGridSize(QSize(96, 96));
        break;
    }
    clearAllIndexWidgets();
    auto metaInfo = FileMetaInfo::fromUri("computer:///");
    if (metaInfo) {
        metaInfo->setMetaInfoInt("peony-qt-desktop-zoom-level", int(mZoomLevel));
    } else {

    }
}

void DesktopIconView::setCutFiles(const QStringList &uris)
{
    ClipbordUtils::setClipboardFiles(uris, true);
}

void DesktopIconView::scrollToSelection(const QString &uri)
{

}

void DesktopIconView::setSelections(const QStringList &uris)
{
    clearSelection();
    for (int i = 0; i < mProxyModel->rowCount(); i++) {
        auto index = mProxyModel->index(i, 0);
        if (uris.contains(index.data(Qt::UserRole).toString())) {
            selectionModel()->select(index, QItemSelectionModel::Select);
        }
    }
}

void DesktopIconView::saveItemPositionInfo(const QString &uri)
{
    auto index = mProxyModel->mapFromSource(mModel->indexFromUri(uri));
    auto indexRect = QListView::visualRect(index);
    QStringList topLeft;
    topLeft<<QString::number(indexRect.top());
    topLeft<<QString::number(indexRect.left());
    auto metaInfo = FileMetaInfo::fromUri(index.data(Qt::UserRole).toString());
    if (metaInfo) {
        metaInfo->setMetaInfoStringList(ITEM_POS_ATTRIBUTE, topLeft);
    }
}

void DesktopIconView::resetItemPosistionInfo(const QString &uri)
{
    auto metaInfo = FileMetaInfo::fromUri(uri);
    if (metaInfo)
        metaInfo->removeMetaInfo(ITEM_POS_ATTRIBUTE);
}

void DesktopIconView::open(const QStringList &uris, bool newWindow)
{

}

void DesktopIconView::updateItemPosistions(const QString &uri)
{
    if (uri.isNull()) {
        for (int i = 0; i < mProxyModel->rowCount(); i++) {
            auto index = mProxyModel->index(i, 0);
            auto metaInfo = FileMetaInfo::fromUri(index.data(Qt::UserRole).toString());
            if (metaInfo) {
                updateItemPosistions(index.data(Qt::UserRole).toString());
            }
        }
        return;
    }

    auto index = mProxyModel->mapFromSource(mModel->indexFromUri(uri));
    //qDebug()<<"update"<<uri<<index.data();

    if (!index.isValid()) {
        //qDebug()<<"err: index invalid";
        return;
    }
    auto metaInfo = FileMetaInfo::fromUri(uri);
    if (!metaInfo) {
        //qDebug()<<"err: no meta data";
        return;
    }

    auto list = metaInfo->getMetaInfoStringList(ITEM_POS_ATTRIBUTE);
    if (!list.isEmpty()) {
        if (list.count() == 2) {
            int top = list.first().toInt();
            int left = list.at(1).toInt();
            if (top >= 0 && left >= 0) {
//                auto rect = visualRect(index);
//                auto grid = gridSize();
//                if (abs(rect.top() - top) < grid.width() && abs(rect.left() - left))
//                    return;
                QPoint p(left, top);
                //qDebug()<<"set"<<index.data()<<p;
                setPositionForIndex(QPoint(left, top), index);
            } else {
                saveItemPositionInfo(uri);
            }
        }
    }
}

bool DesktopIconView::isItemsOverlapped()
{
    QList<QRect> itemRects;
    if (model()) {
        for (int i = 0; i < model()->rowCount(); i++) {
            auto index = model()->index(i, 0);
            auto rect = visualRect(index);
            if (itemRects.contains(rect))
                return true;
            itemRects<<visualRect(index);
        }
    }

    return false;
}

void DesktopIconView::dropEvent(QDropEvent *e)
{
    mRealDoEdit = false;
    //qDebug()<<"drop event";
    /*!
      \todo
      fix the bug that move drop action can not move the desktop
      item to correct position.

      i use copy action to avoid this bug, but the drop indicator
      is incorrect.
      */
    mEditTriggerTimer.stop();
    if (this == e->source()) {

        auto index = indexAt(e->pos());
        if (index.isValid()) {
            auto info = FileInfo::fromUri(index.data(Qt::UserRole).toString());
            if (!info->isDir())
                return;
        }

        QListView::dropEvent(e);

        QHash<QModelIndex, QRect> currentIndexesRects;
        for (int i = 0; i < mProxyModel->rowCount(); i++) {
            auto tmp = mProxyModel->index(i, 0);
            currentIndexesRects.insert(tmp, visualRect(tmp));
        }

        //fixme: handle overlapping.
        if (!mDragIndexes.isEmpty()) {
            QModelIndexList overlappedIndexes;
            for (auto value : currentIndexesRects.values()) {
                auto keys = currentIndexesRects.keys(value);
                if (keys.count() > 1) {
                    for (auto key : keys) {
                        if (mDragIndexes.contains(key) && !overlappedIndexes.contains(key)) {
                            overlappedIndexes<<key;
                        }
                    }
                }
            }

            auto grid = this->gridSize();
            auto viewRect = this->rect();

            QRegion notEmptyRegion;
            for (auto rect : currentIndexesRects) {
                notEmptyRegion += rect;
            }

            for (auto dragedIndex : overlappedIndexes) {
                auto indexRect = QListView::visualRect(dragedIndex);
                if (notEmptyRegion.contains(indexRect.center())) {
                    // move index to closest empty grid.
                    auto next = indexRect;
                    bool isEmptyPos = false;
                    while (!isEmptyPos) {
                        next.translate(0, grid.height());
                        if (next.bottom() > viewRect.bottom()) {
                            int top = next.y();
                            while (true) {
                                if (top < gridSize().height()) {
                                    break;
                                }
                                top-=gridSize().height();
                            }
                            //put item to next column first column
                            next.moveTo(next.x() + grid.width(), top);
                        }
                        if (notEmptyRegion.contains(next.center()))
                            continue;

                        isEmptyPos = true;
                        setPositionForIndex(next.topLeft(), dragedIndex);
                        notEmptyRegion += next;
                    }
                }
            }
        }

        mDragIndexes.clear();

        auto urls = e->mimeData()->urls();
        for (auto url : urls) {
//            if (url.path() == QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
//                continue;
            saveItemPositionInfo(url.toDisplayString());
        }
        return;
    }
    mModel->dropMimeData(e->mimeData(), Qt::MoveAction, -1, -1, this->indexAt(e->pos()));
}

void DesktopIconView::wheelEvent(QWheelEvent *e)
{
    if (QApplication::keyboardModifiers() == Qt::ControlModifier)
    {
        if (e->delta() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        resetAllItemPositionInfos();
    }
}

void DesktopIconView::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Up: {
        if (getSelections().isEmpty()) {
            selectionModel()->select(model()->index(0, 0), QItemSelectionModel::SelectCurrent);
        } else {
            auto index = selectionModel()->selectedIndexes().first();
            auto center = visualRect(index).center();
            auto up = center - QPoint(0, gridSize().height());
            auto upIndex = indexAt(up);
            if (upIndex.isValid()) {
                clearAllIndexWidgets();
                selectionModel()->select(upIndex, QItemSelectionModel::SelectCurrent);
                auto delegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
                setIndexWidget(upIndex, new DesktopIndexWidget(delegate, viewOptions(), upIndex, this));
            }
        }
        return;
    }
    case Qt::Key_Down: {
        if (getSelections().isEmpty()) {
            selectionModel()->select(model()->index(0, 0), QItemSelectionModel::SelectCurrent);
        } else {
            auto index = selectionModel()->selectedIndexes().first();
            auto center = visualRect(index).center();
            auto down = center + QPoint(0, gridSize().height());
            auto downIndex = indexAt(down);
            if (downIndex.isValid()) {
                clearAllIndexWidgets();
                selectionModel()->select(downIndex, QItemSelectionModel::SelectCurrent);
                auto delegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
                setIndexWidget(downIndex, new DesktopIndexWidget(delegate, viewOptions(), downIndex, this));
            }
        }
        return;
    }
    case Qt::Key_Left: {
        if (getSelections().isEmpty()) {
            selectionModel()->select(model()->index(0, 0), QItemSelectionModel::SelectCurrent);
        } else {
            auto index = selectionModel()->selectedIndexes().first();
            auto center = visualRect(index).center();
            auto left = center - QPoint(gridSize().width(), 0);
            auto leftIndex = indexAt(left);
            if (leftIndex.isValid()) {
                clearAllIndexWidgets();
                selectionModel()->select(leftIndex, QItemSelectionModel::SelectCurrent);
                auto delegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
                setIndexWidget(leftIndex, new DesktopIndexWidget(delegate, viewOptions(), leftIndex, this));
            }
        }
        return;
    }
    case Qt::Key_Right: {
        if (getSelections().isEmpty()) {
            selectionModel()->select(model()->index(0, 0), QItemSelectionModel::SelectCurrent);
        } else {
            auto index = selectionModel()->selectedIndexes().first();
            auto center = visualRect(index).center();
            auto right = center + QPoint(gridSize().width(), 0);
            auto rightIndex = indexAt(right);
            if (rightIndex.isValid()) {
                clearAllIndexWidgets();
                selectionModel()->select(rightIndex, QItemSelectionModel::SelectCurrent);
                auto delegate = qobject_cast<DesktopIconViewDelegate*>(itemDelegate());
                setIndexWidget(rightIndex, new DesktopIndexWidget(delegate, viewOptions(), rightIndex, this));
            }
        }
        return;
    }
    case Qt::Key_Shift:
    case Qt::Key_Control:
        mCtrlOrShiftPressed = true;
        break;
    default:
        return QListView::keyPressEvent(e);
    }
}

void DesktopIconView::resizeEvent(QResizeEvent *e)
{
    QListView::resizeEvent(e);
    refresh();
}

void DesktopIconView::keyReleaseEvent(QKeyEvent *e)
{
    QListView::keyReleaseEvent(e);
    mCtrlOrShiftPressed = false;
}

void DesktopIconView::focusOutEvent(QFocusEvent *e)
{
    QListView::focusOutEvent(e);
    mCtrlOrShiftPressed = false;
}

void DesktopIconView::mousePressEvent(QMouseEvent *e)
{
    mRealDoEdit = false;

    if (!mCtrlOrShiftPressed) {
        if (!indexAt(e->pos()).isValid()) {
            clearAllIndexWidgets();
            clearSelection();
        } else {
            auto index = indexAt(e->pos());
            clearAllIndexWidgets();
            mLastIndex = index;
            if (!indexWidget(mLastIndex)) {
                setIndexWidget(mLastIndex,
                               new DesktopIndexWidget(qobject_cast<DesktopIconViewDelegate *>(itemDelegate()), viewOptions(), mLastIndex));
            }
        }
    }

    if (e->button() != Qt::LeftButton) {
        return;
    }

    QListView::mousePressEvent(e);
}

void DesktopIconView::dragMoveEvent(QDragMoveEvent *e)
{
    mRealDoEdit = false;
    auto index = indexAt(e->pos());
    if (index.isValid() && index != mLastIndex) {
        QHoverEvent he(QHoverEvent::HoverMove, e->posF(), e->posF());
        viewportEvent(&he);
    } else {
        QHoverEvent he(QHoverEvent::HoverLeave, e->posF(), e->posF());
        viewportEvent(&he);
    }
    if (e->isAccepted())
        return;
    //qDebug()<<"drag move event";
    if (this == e->source()) {
        e->accept();
        return QListView::dragMoveEvent(e);
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void DesktopIconView::mouseReleaseEvent(QMouseEvent *e)
{
    QListView::mouseReleaseEvent(e);
}

void DesktopIconView::dragEnterEvent(QDragEnterEvent *e)
{
    mRealDoEdit = false;
    //qDebug()<<"drag enter event";
    if (e->mimeData()->hasUrls()) {
        e->setDropAction(Qt::MoveAction);
        e->acceptProposedAction();
    }

    if (e->source() == this) {
        mDragIndexes = selectedIndexes();
    }
}

void DesktopIconView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QListView::mouseDoubleClickEvent(event);
    mRealDoEdit = false;
}

void DesktopIconView::setProxy(DirectoryViewProxyIface *proxy)
{
    Q_UNUSED(proxy)
}

QRect DesktopIconView::visualRect(const QModelIndex &index) const
{
    auto rect = QListView::visualRect(index);
    QPoint p(10, 5);

    switch (zoomLevel()) {
    case Small:
        p *= 0.8;
        break;
    case Large:
        p *= 1.2;
        break;
    case Huge:
        p *= 1.4;
        break;
    default:
        break;
    }
    rect.moveTo(rect.topLeft() + p);
    return rect;
}

const QFont DesktopIconView::getViewItemFont(QStyleOptionViewItem *item)
{
    return item->font;
}

DesktopIconView::DesktopIconView(QWidget *parent)
{
    mRefreshTimer.setInterval(500);
    mRefreshTimer.setSingleShot(true);

    installEventFilter(this);

    initShoutCut();
    //initMenu();
    initDoubleClick();

    connect(qApp, &QApplication::paletteChanged, this, [=]() {
        viewport()->update();
    });

    mEditTriggerTimer.setSingleShot(true);
    mEditTriggerTimer.setInterval(3000);
    mLastIndex = QModelIndex();

    setContentsMargins(0, 0, 0, 0);
    setAttribute(Qt::WA_TranslucentBackground);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setStyle(IconViewStyle::getStyle());

    setItemDelegate(new DesktopIconViewDelegate(this));

    setDefaultDropAction(Qt::MoveAction);

    setSelectionMode(QListView::ExtendedSelection);
    setEditTriggers(QListView::NoEditTriggers);
    setViewMode(QListView::IconMode);
    setMovement(QListView::Snap);
    setFlow(QListView::TopToBottom);
    setResizeMode(QListView::Adjust);
    setWordWrap(true);

    setDragDropMode(QListView::DragDrop);

    //setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(QListView::ExtendedSelection);

    auto zoomLevel = this->zoomLevel();
    setDefaultZoomLevel(zoomLevel);

//#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
//    QTimer::singleShot(500, this, [=](){
//#else
//    QTimer::singleShot(500, [=](){
//#endif
//        connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &selection, const QItemSelection &deselection){
//            //qDebug()<<"selection changed";
//            mRealDoEdit = false;
//            this->setIndexWidget(mLastIndex, nullptr);
//            auto currentSelections = this->selectionModel()->selection().indexes();

//            if (currentSelections.count() == 1) {
//                //qDebug()<<"set index widget";
//                mLastIndex = currentSelections.first();
//                auto delegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
//                this->setIndexWidget(mLastIndex, new DesktopIndexWidget(delegate, viewOptions(), mLastIndex, this));
//            } else {
//                mLastIndex = QModelIndex();
//                for (auto index : deselection.indexes()) {
//                    this->setIndexWidget(index, nullptr);
//                }
//            }
//        });
//    });

    mModel = new DesktopItemModel(this);
    mProxyModel = new DesktopItemProxyModel(mModel);

    mProxyModel->setSourceModel(mModel);

    //connect(mModel, &DesktopItemModel::dataChanged, this, &DesktopIconView::clearAllIndexWidgets);

    connect(mModel, &DesktopItemModel::refreshed, this, [=]() {
        CT_SYSLOG (LOG_DEBUG, "desktop recive refreshed signal");
        this->updateItemPosistions(nullptr);
        this->mIsRefreshing = false;
        if (isItemsOverlapped()) {
            QHash<QModelIndex, QRect> currentIndexesRects;
            for (int i = 0; i < mProxyModel->rowCount(); i++) {
                auto tmp = mProxyModel->index(i, 0);
                currentIndexesRects.insert(tmp, visualRect(tmp));
            }

            QModelIndexList overlappedIndexes;
            for (auto value : currentIndexesRects.values()) {
                auto keys = currentIndexesRects.keys(value);
                if (keys.count() > 1) {
                    keys.removeFirst();
                    overlappedIndexes<<keys;
                }
            }

            auto grid = this->gridSize();
            auto viewRect = this->rect();

            QRegion notEmptyRegion;
            for (auto rect : currentIndexesRects) {
                notEmptyRegion += rect;
            }

            for (auto overrlappedIndex : overlappedIndexes) {
                auto indexRect = QListView::visualRect(overrlappedIndex);
                if (notEmptyRegion.contains(indexRect.center())) {
                    auto next = indexRect;
                    bool isEmptyPos = false;
                    while (!isEmptyPos) {
                        next.translate(0, grid.height());
                        if (next.bottom() > viewRect.bottom()) {
                            int top = next.y();
                            while (true) {
                                if (top < gridSize().height()) {
                                    break;
                                }
                                top-=gridSize().height();
                            }
                            next.moveTo(next.x() + grid.width(), top);
                        }
                        if (notEmptyRegion.contains(next.center()))
                            continue;

                        isEmptyPos = true;
                        setPositionForIndex(next.topLeft(), overrlappedIndex);
                        notEmptyRegion += next;
                    }
                }
            }

            saveAllItemPosistionInfos();
        }
    });

    connect(mModel, &DesktopItemModel::requestClearIndexWidget, this, &DesktopIconView::clearAllIndexWidgets);

    connect(mModel, &DesktopItemModel::requestLayoutNewItem, this, [=](const QString &uri) {
        CT_SYSLOG(LOG_DEBUG, "recive DesktopItemModel::requestLayoutNewItem signal");
        auto index = mProxyModel->mapFromSource(mModel->indexFromUri(uri));
        auto rect = QRect(QPoint(0, 0), gridSize());
        rect.moveTo(this->contentsMargins().left(), this->contentsMargins().top());

        auto grid = this->gridSize();
        auto viewRect = this->rect();
        auto next = rect;
        while (true) {
            if (this->indexAt(next.center()).data(Qt::UserRole).toString() == uri) {
                this->setPositionForIndex(next.topLeft(), index);
                this->saveItemPositionInfo(uri);
                return;
            }
            if (!this->indexAt(next.center()).isValid()) {
                CT_SYSLOG(LOG_DEBUG, "put %s position x:%.2f; y:%.2f", index.data().data(), next.topLeft().x(), next.topLeft().y());
                this->setPositionForIndex(next.topLeft(), index);
                this->saveItemPositionInfo(uri);
                break;
            }
            next = QListView::visualRect(this->indexAt(next.center()));
            next.translate(0, grid.height());
            if (next.bottom() > viewRect.bottom()) {
                int top = next.y();
                while (true) {
                    if (top < gridSize().height()) {
                        break;
                    }
                    top-=gridSize().height();
                }
                next.moveTo(next.x() + grid.width(), top);
            }
        }
    });

    connect(mModel, &DesktopItemModel::requestUpdateItemPositions, this, &DesktopIconView::updateItemPosistions);

    connect(mModel, &DesktopItemModel::fileCreated, this, [=](const QString &uri) {
        CT_SYSLOG(LOG_DEBUG, "recive DesktopItemModel::fileCreated signal");
        if (mNewFilesToBeSelected.isEmpty()) {
            mNewFilesToBeSelected<<uri;
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
            QTimer::singleShot(500, this, [=]() {
#else
            QTimer::singleShot(500, [=]() {
#endif
                if (this->state() & QAbstractItemView::EditingState)
                    return;
                this->setSelections(mNewFilesToBeSelected);
                mNewFilesToBeSelected.clear();
            });
        } else {
            if (!mNewFilesToBeSelected.contains(uri)) {
                mNewFilesToBeSelected<<uri;
            }
        }
        refresh();
    });

    connect(mProxyModel, &QSortFilterProxyModel::layoutChanged, this, [=]() {
        if (mProxyModel->getSortType() == DesktopItemProxyModel::Other) {
            return;
        }
        if (mProxyModel->sortColumn() != 0) {
            return;
        }
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
        QTimer::singleShot(100, this, [=]() {
#else
        QTimer::singleShot(100, [=]() {
#endif
            this->saveAllItemPosistionInfos();
        });
    });

    connect(this, &QListView::iconSizeChanged, this, [=]() {
        this->setSortType(GlobalSettings::getInstance()->getValue(LAST_DESKTOP_SORT_ORDER).toInt());
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
        QTimer::singleShot(100, this, [=]() {
#else
        QTimer::singleShot(100, [=]() {
#endif
            this->saveAllItemPosistionInfos();
        });
    });

    setModel(mProxyModel);

    this->refresh();
}

DesktopIconView::~DesktopIconView()
{

}

void DesktopIconView::initMenu()
{

}

int DesktopIconView::getSortType()
{
    return mProxyModel->getSortType();
}

int DesktopIconView::getSortOrder()
{
    return mProxyModel->sortOrder();
}

void DesktopIconView::initShoutCut()
{
    CT_SYSLOG(LOG_DEBUG, "init shortcut");
    QAction *copyAction = new QAction(this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, [=]() {
        auto selectedUris = this->getSelections();
        if (!selectedUris.isEmpty())
            ClipbordUtils::setClipboardFiles(selectedUris, false);
    });
    addAction(copyAction);

    QAction *cutAction = new QAction(this);
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, [=]() {
        auto selectedUris = this->getSelections();
        if (!selectedUris.isEmpty())
            ClipbordUtils::setClipboardFiles(selectedUris, true);
    });
    addAction(cutAction);

    QAction *pasteAction = new QAction(this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, [=]() {
        auto clipUris = ClipbordUtils::getClipboardFilesUris();
        if (ClipbordUtils::isClipboardHasFiles()) {
            ClipbordUtils::pasteClipboardFiles(this->getDirectoryUri());
        }
    });
    addAction(pasteAction);

    QAction *trashAction = new QAction(this);
    trashAction->setShortcut(QKeySequence::Delete);
    connect(trashAction, &QAction::triggered, [=]() {
        auto selectedUris = this->getSelections();
        if (!selectedUris.isEmpty()) {
            clearAllIndexWidgets();
            auto op = new FileTrashOperation(selectedUris);
            FileOperationManager::getInstance()->slotStartOperation(op, true);
        }
    });
    addAction(trashAction);

    QAction *undoAction = new QAction(this);
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered,
    [=]() {
        FileOperationManager::getInstance()->slotUndo();
    });
    addAction(undoAction);

    QAction *redoAction = new QAction(this);
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered,
    [=]() {
        FileOperationManager::getInstance()->slotRedo();
    });
    addAction(redoAction);

    QAction *zoomInAction = new QAction(this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, [=]() {
        this->zoomIn();
    });
    addAction(zoomInAction);

    QAction *zoomOutAction = new QAction(this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, [=]() {
        this->zoomOut();
    });
    addAction(zoomOutAction);

    QAction *renameAction = new QAction(this);
    renameAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_E));
    connect(renameAction, &QAction::triggered, [=]() {
        auto selections = this->getSelections();
        if (selections.count() == 1) {
            this->editUri(selections.first());
        }
    });
    addAction(renameAction);

    QAction *removeAction = new QAction(this);
    removeAction->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete));
    connect(removeAction, &QAction::triggered, [=]() {
        this->getSelections();
        clearAllIndexWidgets();
        FileOperationUtils::executeRemoveActionWithDialog(this->getSelections());
    });
    addAction(removeAction);

    auto propertiesWindowAction = new QAction(this);
    propertiesWindowAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::ALT + Qt::Key_Return)
                                         <<QKeySequence(Qt::ALT + Qt::Key_Enter));
    connect(propertiesWindowAction, &QAction::triggered, this, [=]() {
        if (this->getSelections().count() > 0)
        {
            PropertiesWindow *w = new PropertiesWindow(this->getSelections());
            w->show();
        }
    });
    addAction(propertiesWindowAction);

    auto newFolderAction = new QAction(this);
    newFolderAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));
    connect(newFolderAction, &QAction::triggered, this, [=]() {
        FileCreateTemplOperation op(this->getDirectoryUri(), FileCreateTemplOperation::EmptyFolder, tr("New Folder"));
        op.run();
        auto targetUri = op.target();
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
        QTimer::singleShot(500, this, [=]() {
#else
        QTimer::singleShot(500, [=]() {
#endif
            this->scrollToSelection(targetUri);
        });
    });
    addAction(newFolderAction);

    auto refreshAction = new QAction(this);
    refreshAction->setShortcut(Qt::Key_F5);
    connect(refreshAction, &QAction::triggered, this, [=]() {
        this->refresh();
    });
    addAction(refreshAction);

    QAction *editAction = new QAction(this);
    editAction->setShortcuts(QList<QKeySequence>()<<QKeySequence(Qt::ALT + Qt::Key_E)<<Qt::Key_F2);
    connect(editAction, &QAction::triggered, this, [=]() {
        auto selections = this->getSelections();
        if (selections.count() == 1) {
            this->editUri(selections.first());
        }
    });
    addAction(editAction);
}

void DesktopIconView::initDoubleClick()
{
    connect(this, &QListView::doubleClicked, this, [=](const QModelIndex &index) {
        index.data(FileItemModel::UriRole);
        auto uri = index.data(FileItemModel::UriRole).toString();
        auto info = FileInfo::fromUri(uri, false);
        auto job = new FileInfoJob(info);
        job->setAutoDelete();
        job->connect(job, &FileInfoJob::queryAsyncFinished, [=]() {
            if (info->isDir() || info->isVolume() || info->isVirtual()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
                QProcess p;
                QUrl url = uri;
                p.setProgram("graceful-desktop");
                p.setArguments(QStringList() << url.toEncoded() <<"%U&");
                p.startDetached();
#else
                QProcess p;
                p.startDetached("peony", QStringList()<<uri<<"%U&");
#endif
            } else {
                FileLaunchManager::openAsync(uri, false, false);
            }
            this->clearSelection();
        });
        job->queryAsync();
    }, Qt::UniqueConnection);
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
    QStringList uris;
    auto indexes = selectionModel()->selection().indexes();
    for (auto index : indexes) {
        uris << index.data(Qt::UserRole).toString();
    }
    return uris;
}

const QStringList DesktopIconView::getAllFileUris()
{

}

bool DesktopIconView::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::StyleChange) {
        if (mModel)
            updateItemPosistions();
    }
    return false;
}
