#include "desktop-item-model.h"


DesktopItemModel::DesktopItemModel(QObject *parent)
{

}

DesktopItemModel::~DesktopItemModel()
{

}

const QString DesktopItemModel::indexUri(const QModelIndex &index)
{

}

const QModelIndex DesktopItemModel::indexFromUri(const QString &uri)
{

}

Qt::DropActions DesktopItemModel::supportedDropActions() const
{

}

Qt::ItemFlags DesktopItemModel::flags(const QModelIndex &index) const
{

}

QMimeData *DesktopItemModel::mimeData(const QModelIndexList &indexes) const
{

}

bool DesktopItemModel::removeRow(int row, const QModelIndex &parent)
{

}

bool DesktopItemModel::insertRow(int row, const QModelIndex &parent)
{

}

int DesktopItemModel::rowCount(const QModelIndex &parent) const
{

}

QVariant DesktopItemModel::data(const QModelIndex &index, int role) const
{

}

bool DesktopItemModel::insertRows(int row, int count, const QModelIndex &parent)
{

}

bool DesktopItemModel::removeRows(int row, int count, const QModelIndex &parent)
{

}

bool DesktopItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{

}

void DesktopItemModel::refresh()
{

}

void DesktopItemModel::onEnumerateFinished()
{

}
