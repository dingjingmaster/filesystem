#include "filelabel-model.h"


FileLabelModel *FileLabelModel::getGlobalModel()
{

}

int FileLabelModel::lastLabelId()
{

}

void FileLabelModel::removeLabel(int id)
{

}

const QStringList FileLabelModel::getLabels()
{

}

const QList<QColor> FileLabelModel::getColors()
{

}

FileLabelItem *FileLabelModel::itemFromId(int id)
{

}

QList<FileLabelItem *> FileLabelModel::getAllFileLabelItems()
{

}

void FileLabelModel::setLabelName(int id, const QString &name)
{

}

void FileLabelModel::setLabelColor(int id, const QColor &color)
{

}

const QStringList FileLabelModel::getFileLabels(const QString &uri)
{

}

void FileLabelModel::addLabelToFile(const QString &uri, int labelId)
{

}

const QList<int> FileLabelModel::getFileLabelIds(const QString &uri)
{

}

const QList<QColor> FileLabelModel::getFileColors(const QString &uri)
{

}

FileLabelItem *FileLabelModel::itemFormIndex(const QModelIndex &index)
{

}

void FileLabelModel::addLabel(const QString &label, const QColor &color)
{

}

void FileLabelModel::removeFileLabel(const QString &uri, int labelId)
{

}

Qt::ItemFlags FileLabelModel::flags(const QModelIndex &index) const
{

}

int FileLabelModel::rowCount(const QModelIndex &parent) const
{

}

QVariant FileLabelModel::data(const QModelIndex &index, int role) const
{

}

bool FileLabelModel::insertRows(int row, int count, const QModelIndex &parent)
{

}

bool FileLabelModel::removeRows(int row, int count, const QModelIndex &parent)
{

}

bool FileLabelModel::setData(const QModelIndex &index, const QVariant &value, int role)
{

}

void FileLabelModel::setName(FileLabelItem *item, const QString &name)
{

}

void FileLabelModel::setColor(FileLabelItem *item, const QColor &color)
{

}

void FileLabelModel::addId()
{

}

void FileLabelModel::initLabelItems()
{

}

FileLabelModel::FileLabelModel(QObject *parent) : QAbstractListModel(parent)
{

}

FileLabelModel::~FileLabelModel()
{

}
