#include "side-bar.h"

#include <QApplication>

SideBar::SideBar(QWidget *parent) : QTreeView(parent)
{
    setDropIndicatorShown(false);
    setAttribute(Qt::WA_Hover);

    connect(qApp, &QApplication::paletteChanged, this, [=]() {
        this->update();
        this->viewport()->update();
    });

    setContextMenuPolicy(Qt::CustomContextMenu);

    setDragDropMode(QTreeView::DragDrop);

    setIndentation(15);
    setSelectionBehavior(QTreeView::SelectRows);
    setContentsMargins(0, 0, 0, 0);

    setItemDelegate(new SideBarDelegate(this));

    auto model = new SideBarModel(this);
    auto proxy_model = new SideBarProxyFilterSortModel(model);

    setSortingEnabled(true);
    setExpandsOnDoubleClick(false);
    //don't show HorizontalScroll
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setVisible(false);

    setModel(proxy_model);

    proxy_model->setSourceModel(model);

    connect(this, &QTreeView::expanded, [=](const QModelIndex &index) {
        auto item = proxy_model->itemFromIndex(index);
        item->findChildrenAsync();
    });

    connect(this, &QTreeView::collapsed, [=](const QModelIndex &index) {
        auto item = proxy_model->itemFromIndex(index);
        item->clearChildren();
    });

    connect(this, &QTreeView::clicked, [=](const QModelIndex &index) {
        switch (index.column()) {
        case 0: {
            auto item = proxy_model->itemFromIndex(index);
            //some side bar item doesn't have a uri.
            //do not emit signal with a null uri to window.
            if (!item->uri().isNull())
                Q_EMIT this->updateWindowLocationRequest(item->uri());
            break;
        }
        case 1: {
            auto item = proxy_model->itemFromIndex(index);
            if (item->isMounted()) {
                auto leftIndex = proxy_model->index(index.row(), 0, index.parent());
                this->collapse(leftIndex);
                item->unmount();
            }
            break;
        }
        default:
            break;
        }
    });

    connect(this, &QTreeView::customContextMenuRequested, this, [=](const QPoint &pos) {
        auto index = indexAt(pos);
        auto item = proxy_model->itemFromIndex(index);
        if (item) {
            if (item->type() != SideBarAbstractItem::SeparatorItem) {
                SideBarMenu menu(item, this);
                menu.exec(QCursor::pos());
            }
        }
    });

    expandAll();
}

QSize SideBar::sizeHint() const
{
    auto size = QTreeView::sizeHint();
    size.setWidth(180);
    return size;
}

void SideBar::paintEvent(QPaintEvent *e)
{
    QTreeView::paintEvent(e);
}

QRect SideBar::visualRect(const QModelIndex &index) const
{
    return QTreeView::visualRect(index);
}

void SideBar::dragEnterEvent(QDragEnterEvent *e)
{
    e->accept();
    setState(DraggingState);
}

void SideBar::dragMoveEvent(QDragMoveEvent *e)
{
    auto widget = static_cast<QWidget*>(e->source());
    if (widget) {
        if (widget->topLevelWidget() == this->topLevelWidget()) {
            QTreeView::dragMoveEvent(e);
        }
    }

    e->accept();
}
