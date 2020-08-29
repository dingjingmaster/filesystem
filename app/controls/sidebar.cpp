#include "sidebar.h"

#include <QMouseEvent>
#include <QHeaderView>
#include <QApplication>
#include "menu/sidebar-menu.h"
#include <model/sidebar-model.h>
#include <delegate/sidebar-delegate.h>
#include <model/sidebar-proxy-filter-sort-model.h>

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
    auto proxyModel = new SideBarProxyFilterSortModel(model);

    setSortingEnabled(true);
    setExpandsOnDoubleClick(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setVisible(false);

    setModel(proxyModel);

    proxyModel->setSourceModel(model);

    connect(this, &QTreeView::expanded, [=](const QModelIndex &index) {
        auto item = proxyModel->itemFromIndex(index);
        item->findChildrenAsync();
    });

    connect(this, &QTreeView::collapsed, [=](const QModelIndex &index) {
        auto item = proxyModel->itemFromIndex(index);
        item->clearChildren();
    });

    connect(this, &QTreeView::clicked, [=](const QModelIndex &index) {
        switch (index.column()) {
        case 0: {
            auto item = proxyModel->itemFromIndex(index);
            //some side bar item doesn't have a uri.
            //do not emit signal with a null uri to window.
            if (!item->uri().isNull())
                Q_EMIT this->updateWindowLocationRequest(item->uri());
            break;
        }
        case 1: {
            auto item = proxyModel->itemFromIndex(index);
            if (item->isMounted()) {
                auto leftIndex = proxyModel->index(index.row(), 0, index.parent());
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
        auto item = proxyModel->itemFromIndex(index);
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
