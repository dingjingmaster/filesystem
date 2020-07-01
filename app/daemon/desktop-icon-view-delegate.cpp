#include "desktop-icon-view-delegate.h"


DesktopIconViewDelegate::DesktopIconViewDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

DesktopIconViewDelegate::~DesktopIconViewDelegate()
{

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

}

QWidget *DesktopIconViewDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

}

void DesktopIconViewDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

}
