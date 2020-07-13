#include "navigation-sidebar.h"
#include "sidebar-menu.h"

#include <QEvent>
#include <QPainter>
#include <QKeyEvent>
#include <QScrollBar>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <global-settings.h>

#include <model/sidebar-model.h>
#include <model/sidebar-proxy-filter-sort-model.h>

NavigationSideBar::NavigationSideBar(QWidget *parent) : QTreeView(parent)
{
    setIconSize(QSize(16, 16));

    setProperty("useIconHighlightEffect", true);
    setProperty("iconHighlightEffectMode", 1);

    this->verticalScrollBar()->setProperty("drawScrollBarGroove", false);

    setDragDropMode(QTreeView::DropOnly);

    setProperty("doNotBlur", true);
    viewport()->setProperty("doNotBlur", true);

    auto delegate = new NavigationSideBarItemDelegate(this);
    setItemDelegate(delegate);

    installEventFilter(this);

    setStyleSheet(".NavigationSideBar"
                  "{"
                  "border: 0px solid transparent"
                  "}");
    setAttribute(Qt::WA_TranslucentBackground);
    viewport()->setAttribute(Qt::WA_TranslucentBackground);
    header()->setSectionResizeMode(QHeaderView::Custom);
    header()->hide();

    setContextMenuPolicy(Qt::CustomContextMenu);

    setExpandsOnDoubleClick(false);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mModel = new SideBarModel(this);
    mProxyModel = new SideBarProxyFilterSortModel(this);
    mProxyModel->setSourceModel(mModel);

//    this->setModel(mProxyModel);

//    connect(this, &QTreeView::expanded, [=](const QModelIndex &index) {
//        auto item = mProxyModel->itemFromIndex(index);
//        item->findChildrenAsync();
//    });

//    connect(this, &QTreeView::collapsed, [=](const QModelIndex &index) {
//        auto item = mProxyModel->itemFromIndex(index);
//        item->clearChildren();
//    });

//    connect(this, &QTreeView::clicked, [=](const QModelIndex &index) {
//        switch (index.column()) {
//        case 0: {
//            auto item = mProxyModel->itemFromIndex(index);
//            if (!item->uri().isNull()) {
//                Q_EMIT this->updateWindowLocationRequest(item->uri());
//            }
//            break;
//        }
//        case 1: {
//            auto item = mProxyModel->itemFromIndex(index);
//            if (item->isMounted()) {
//                auto leftIndex = mProxyModel->index(index.row(), 0, index.parent());
//                this->collapse(leftIndex);
//                item->unmount();
//            }
//            break;
//        }
//        default:
//            break;
//        }
//    });

//    connect(this, &QTreeView::customContextMenuRequested, this, [=](const QPoint &pos) {
//        auto index = indexAt(pos);
//        auto item = mProxyModel->itemFromIndex(index);
//        if (item) {
//            if (item->type() != SideBarAbstractItem::SeparatorItem) {
//                SideBarMenu menu(item, nullptr);
//                menu.exec(QCursor::pos());
//            }
//        }
//    });

//    expandAll();
}

bool NavigationSideBar::eventFilter(QObject *obj, QEvent *e)
{
    return false;
}

void NavigationSideBar::updateGeometries()
{
    setViewportMargins(4, 0, 4, 0);
    QTreeView::updateGeometries();
}

void NavigationSideBar::scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint hint)
{
    if (index.isValid()) {
        if (index.column() == 0) {
            QTreeView::scrollTo(index, hint);
        }
    }
}

void NavigationSideBar::paintEvent(QPaintEvent *event)
{
    QTreeView::paintEvent(event);
}

void NavigationSideBar::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);
    if (header()->count() > 0)
        header()->resizeSection(0, this->viewport()->width() - 30);
}

QSize NavigationSideBar::sizeHint() const
{
    return QTreeView::sizeHint();
}

void NavigationSideBar::keyPressEvent(QKeyEvent *event)
{
    QTreeView::keyPressEvent(event);

    if (event->key() == Qt::Key_Return) {
        if (!selectedIndexes().isEmpty()) {
            auto index = selectedIndexes().first();
            auto uri = index.data(Qt::UserRole).toString();
            Q_EMIT this->updateWindowLocationRequest(uri, true);
        }
    }
}

NavigationSideBarItemDelegate::NavigationSideBarItemDelegate(QObject *parent)
{

}

QSize NavigationSideBarItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(36);
    return size;
}

NavigationSideBarContainer::NavigationSideBarContainer(QWidget *parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    mLayout = new QVBoxLayout;
    mLayout->setContentsMargins(0, 4, 0, 0);
    mLayout->setSpacing(0);
}

void NavigationSideBarContainer::addSideBar(NavigationSideBar *sidebar)
{
    if (mSidebar) {
        return;
    }

    mSidebar = sidebar;
    mLayout->addWidget(sidebar);

    QWidget *w = new QWidget(this);
    QVBoxLayout *l = new QVBoxLayout;
    l->setContentsMargins(4, 4, 2, 4);

    mLabelButton = new QPushButton(QIcon::fromTheme("emblem-important-symbolic"), tr("All tags..."), this);
    mLabelButton->setProperty("useIconHighlightEffect", true);
    mLabelButton->setProperty("iconHighlightEffectMode", 1);
    mLabelButton->setProperty("fillIconSymbolicColor", true);
    mLabelButton->setCheckable(true);

    l->addWidget(mLabelButton);

    connect(mLabelButton, &QPushButton::clicked, mSidebar, &NavigationSideBar::labelButtonClicked);

    w->setLayout(l);

    mLayout->addWidget(w);

    setLayout(mLayout);
}

QSize NavigationSideBarContainer::sizeHint() const
{
    auto size = QWidget::sizeHint();
    auto width = GlobalSettings::getInstance()->getValue(DEFAULT_SIDEBAR_WIDTH).toInt();
    size.setWidth(width);
    return size;
}
