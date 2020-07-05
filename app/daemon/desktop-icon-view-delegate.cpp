#include "desktop-icon-view-delegate.h"
#include "desktop-icon-view.h"

#include <QPainter>

#include <delegate/icon-view-delegate.h>


DesktopIconViewDelegate::DesktopIconViewDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    mStyledButton = new QPushButton;
}

DesktopIconViewDelegate::~DesktopIconViewDelegate()
{
    mStyledButton->deleteLater();
}

DesktopIconView *DesktopIconViewDelegate::getView() const
{

}

void DesktopIconViewDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    return QStyledItemDelegate::initStyleOption(option, index);
}

QSize DesktopIconViewDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{

}

void DesktopIconViewDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{

}

void DesktopIconViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{

}

void DesktopIconViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    auto view = qobject_cast<DesktopIconView*>(parent());

    auto style = option.widget->style();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if (view->state() == DesktopIconView::DraggingState) {
        if (auto widget = view->indexWidget(index)) {
            view->setIndexWidget(index, nullptr);
        }

        if (view->selectionModel()->selection().contains(index)) {
            painter->setOpacity(0.8);
        }
    } else if (opt.state.testFlag(QStyle::State_Selected)) {
        if (view->indexWidget(index)) {
            opt.text = nullptr;
        }
    }

    // 绘制桌面背景
    if (!view->indexWidget(index)) {
        //painter->setClipRect(opt.rect);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (opt.state.testFlag(QStyle::State_MouseOver) && !opt.state.testFlag(QStyle::State_Selected)) {
            QColor color = mStyledButton->palette().highlight().color();
            color.setAlpha(255*0.3);
            //painter->fillRect(opt.rect, color);
            color.setAlpha(255*0.5);
            painter->setPen(color.darker(100));
            painter->setBrush(color);
            painter->drawRoundedRect(opt.rect.adjusted(1, 1, -1, -1), 6, 6);
        }
        if (opt.state.testFlag(QStyle::State_Selected)) {
            QColor color = mStyledButton->palette().highlight().color();
            color.setAlpha(255*0.7);//half transparent
            //painter->fillRect(opt.rect, color);
            color.setAlpha(255*0.8);
            painter->setPen(color);
            painter->setBrush(color);
            painter->drawRoundedRect(opt.rect.adjusted(1, 1, -1, -1), 6, 6);
        }
        painter->restore();
    }

    auto iconSizeExpected = view->iconSize();
    auto iconRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
    int y_delta = iconSizeExpected.height() - iconRect.height();
    opt.rect.translate(0, y_delta);

    int maxTextHight = opt.rect.height() - iconSizeExpected.height() - 10;
    if (maxTextHight < 0) {
        maxTextHight = 0;
    }

    // 绘制桌面图标
    auto color = QColor(230, 230, 230);
    opt.palette.setColor(QPalette::Text, color);
    color.setRgb(240, 240, 240);
    opt.palette.setColor(QPalette::HighlightedText, color);

    auto text = opt.text;
    opt.text = nullptr;

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    opt.text = text;

    painter->save();
    //painter->translate(visualRect.topLeft());
    painter->translate(opt.rect.topLeft());
    painter->translate(0, -y_delta);

    //paint text shadow
    painter->save();
    painter->translate(1, 1 + iconSizeExpected.height() + 10);
    //painter->setFont(opt.font);
    QColor shadow = Qt::black;
    shadow.setAlpha(127);
    painter->setPen(shadow);
    IconViewTextHelper::paintText(painter, opt, index, maxTextHight, 0, 2, false);
    painter->restore();

    //paint text
    painter->save();
    painter->translate(0, 0 + iconSizeExpected.height() + 10);
    //painter->setFont(opt.font);
    painter->setPen(opt.palette.highlightedText().color());
    IconViewTextHelper::paintText(painter, opt, index, maxTextHight, 0, 2, false);
    painter->restore();

    painter->restore();

    // 绘制链接图标
    if (index.data(Qt::UserRole + 1).toBool()) {
        QSize symbolicIconSize = QSize(16, 16);
        int offset = 8;
        switch (view->zoomLevel()) {
        case DesktopIconView::Small: {
            symbolicIconSize = QSize(8, 8);
            offset = 10;
            break;
        }
        case DesktopIconView::Normal: {
            break;
        }
        case DesktopIconView::Large: {
            offset = 4;
            symbolicIconSize = QSize(24, 24);
            break;
        }
        case DesktopIconView::Huge: {
            offset = 2;
            symbolicIconSize = QSize(32, 32);
            break;
        }
        default: {
            break;
        }
        }
        auto topRight = opt.rect.topRight();
        topRight.setX(topRight.x() - offset - symbolicIconSize.width());
        topRight.setY(topRight.y() + offset);
        auto linkRect = QRect(topRight, symbolicIconSize);
        QIcon symbolicLinkIcon = QIcon::fromTheme("emblem-symbolic-link");
        symbolicLinkIcon.paint(painter, linkRect, Qt::AlignCenter);
    }

    painter->restore();
}

QWidget *DesktopIconViewDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

}

void DesktopIconViewDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

}
