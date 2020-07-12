#include "filelabel-model.h"

#include <QMessageBox>
#include "filelabel-model.h"
#include <syslog/clib_syslog.h>
#include <file/file-meta-info.h>

static FileLabelModel *gInstance = nullptr;

FileLabelModel *FileLabelModel::getGlobalModel()
{
    if (!gInstance) {
        gInstance = new FileLabelModel;
    }
    return gInstance;
}

int FileLabelModel::lastLabelId()
{
    mLabelSettings = new QSettings(QSettings::UserScope, "org.graceful", "fm", this);
    if (mLabelSettings->value("lastid").isNull()) {
        CT_SYSLOG (LOG_WARNING, "gsettings not get the key 'lastid'");
        return 0;
    } else {
        return mLabelSettings->value("lastid").toInt();
    }
}

void FileLabelModel::removeLabel(int id)
{
    beginResetModel();

    for (auto item : mLabels) {
        if (item->id() == id) {
            mLabels.removeOne(item);
            item->deleteLater();
            break;
        }
    }

    mLabelSettings->beginWriteArray("labels");
    mLabelSettings->setArrayIndex(id);
    mLabelSettings->setValue("visible", false);
    mLabelSettings->endArray();
    mLabelSettings->sync();

    endResetModel();
}

const QStringList FileLabelModel::getLabels()
{
    QStringList l;

    int size = mLabelSettings->beginReadArray("labels");
    for (int i = 0; i < size; i++) {
        mLabelSettings->setArrayIndex(i);
        if (mLabelSettings->value("visible").toBool()) {
            l << mLabelSettings->value("label").toString();
        }
    }
    mLabelSettings->endArray();

    return l;
}

const QList<QColor> FileLabelModel::getColors()
{
    QList<QColor> l;

    int size = mLabelSettings->beginReadArray("labels");
    for (int i = 0; i < size; i++) {
        mLabelSettings->setArrayIndex(i);
        if (mLabelSettings->value("visible").toBool()) {
            l << qvariant_cast<QColor>(mLabelSettings->value("color"));
        }
    }
    mLabelSettings->endArray();

    return l;
}

FileLabelItem *FileLabelModel::itemFromId(int id)
{
    for (auto item : this->mLabels) {
        if (id == item->id()) {
            return item;
        }
    }
    return nullptr;
}

QList<FileLabelItem *> FileLabelModel::getAllFileLabelItems()
{
    return mLabels;
}

void FileLabelModel::setLabelName(int id, const QString &name)
{
    for (auto item : mLabels) {
        if (item->id() == id) {
            item->setName(name);
            int row = mLabels.indexOf(item);
            Q_EMIT dataChanged(index(row), index(row));
            break;
        }
    }
}

void FileLabelModel::setLabelColor(int id, const QColor &color)
{
    for (auto item : mLabels) {
        if (item->id() == id) {
            item->setColor(color);
            int row = mLabels.indexOf(item);
            Q_EMIT dataChanged(index(row), index(row));
            break;
        }
    }
}

const QStringList FileLabelModel::getFileLabels(const QString &uri)
{
    QStringList l;
    auto metaInfo = FileMetaInfo::fromUri(uri);
    if (! metaInfo || metaInfo->getMetaInfoVariant(FM_FILE_LABEL_IDS).isNull()) {
        return l;
    }
    auto labels = metaInfo->getMetaInfoStringList(FM_FILE_LABEL_IDS);
    for (auto label : labels) {
        auto id = label.toInt();
        auto item = itemFromId(id);
        if (item) {
            l << item->name();
        }
    }
    return l;
}

void FileLabelModel::addLabelToFile(const QString &uri, int labelId)
{
    auto metaInfo = FileMetaInfo::fromUri(uri);
    QStringList labelIds;
    if (metaInfo && !metaInfo->getMetaInfoVariant(FM_FILE_LABEL_IDS).isNull()) {
        labelIds = metaInfo->getMetaInfoStringList(FM_FILE_LABEL_IDS);
    }
    labelIds<<QString::number(labelId);
    labelIds.removeDuplicates();
    metaInfo->setMetaInfoStringList(FM_FILE_LABEL_IDS, labelIds);
    Q_EMIT fileLabelChanged(uri);
}

const QList<int> FileLabelModel::getFileLabelIds(const QString &uri)
{
    QList<int> l;
    auto metaInfo = FileMetaInfo::fromUri(uri);
    if (! metaInfo || metaInfo->getMetaInfoVariant(FM_FILE_LABEL_IDS).isNull())
        return l;
    auto labels = metaInfo->getMetaInfoStringList(FM_FILE_LABEL_IDS);
    for (auto label : labels) {
        l << label.toInt();
    }
    return l;
}

const QList<QColor> FileLabelModel::getFileColors(const QString &uri)
{
    QList<QColor> l;
    auto metaInfo = FileMetaInfo::fromUri(uri);
    if (! metaInfo || metaInfo->getMetaInfoVariant(FM_FILE_LABEL_IDS).isNull()) {
        return l;
    }
    auto labels = metaInfo->getMetaInfoStringList(FM_FILE_LABEL_IDS);
    for (auto label : labels) {
        auto id = label.toInt();
        auto item = itemFromId(id);
        if (item) {
            l << item->color();
        }
    }
    return l;
}

FileLabelItem *FileLabelModel::itemFormIndex(const QModelIndex &index)
{
    if (index.isValid()) {
        return mLabels.at(index.row());
    }
    return nullptr;
}

void FileLabelModel::addLabel(const QString &label, const QColor &color)
{
    beginResetModel();

    if (getLabels().contains(label) || getColors().contains(color)) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Label or color is duplicated."));
        return;
    }

    int lastid = lastLabelId();
    mLabelSettings->beginWriteArray("labels");
    mLabelSettings->setArrayIndex(lastid + 1);
    mLabelSettings->setValue("label", label);
    mLabelSettings->setValue("color", color);
    mLabelSettings->setValue("visible", true);
    mLabelSettings->endArray();

    auto item = new FileLabelItem(this);
    item->mId = lastid + 1;
    item->mName = label;
    item->mColor = color;

    mLabels.append(item);

    addId();

    connect(item, &FileLabelItem::nameChanged, this, [=](const QString &name) {
        mLabelSettings->beginWriteArray("labels");
        mLabelSettings->setArrayIndex(item->id());
        mLabelSettings->setValue("label", name);
        mLabelSettings->endArray();
        mLabelSettings->sync();
    });

    connect(item, &FileLabelItem::colorChanged, this, [=](const QColor &color) {
        mLabelSettings->beginWriteArray("labels");
        mLabelSettings->setArrayIndex(item->id());
        mLabelSettings->setValue("color", color);
        mLabelSettings->endArray();
        mLabelSettings->sync();
    });

    endResetModel();
}

void FileLabelModel::removeFileLabel(const QString &uri, int labelId)
{
    auto metaInfo = FileMetaInfo::fromUri(uri);
    if (! metaInfo) {
        return;
    }
    if (labelId <= 0) {
        metaInfo->removeMetaInfo(FM_FILE_LABEL_IDS);
    } else {
        if (metaInfo->getMetaInfoVariant(FM_FILE_LABEL_IDS).isNull()) {
            return;
        }
        QStringList labelIds = metaInfo->getMetaInfoStringList(FM_FILE_LABEL_IDS);
        labelIds.removeOne(QString::number(labelId));
        metaInfo->setMetaInfoStringList(FM_FILE_LABEL_IDS, labelIds);
    }
    Q_EMIT fileLabelChanged(uri);
}

Qt::ItemFlags FileLabelModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int FileLabelModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return mLabels.size();
}

QVariant FileLabelModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole: {
        return mLabels.at(index.row())->name();
    }
    case Qt::DecorationRole: {
        return mLabels.at(index.row())->color();
    }
    default:
        return QVariant();
    }
}

bool FileLabelModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    endInsertRows();

    return true;
}

bool FileLabelModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    endRemoveRows();

    return true;
}

bool FileLabelModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        auto name = value.toString();
        if (name.isEmpty()) {
            return false;
        }
        if (getLabels().contains(name)) {
            QMessageBox::critical(nullptr, tr("Error"), tr("Label or color is duplicated."));
            return false;
        }
        this->setLabelName(mLabels.at(index.row())->id(), name);
        Q_EMIT dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

void FileLabelModel::setName(FileLabelItem *item, const QString &name)
{
    mLabelSettings->beginWriteArray("labels");
    mLabelSettings->setArrayIndex(item->id());
    mLabelSettings->setValue("label", name);
    mLabelSettings->endArray();
    mLabelSettings->sync();
}

void FileLabelModel::setColor(FileLabelItem *item, const QColor &color)
{
    mLabelSettings->beginWriteArray("labels");
    mLabelSettings->setArrayIndex(item->id());
    mLabelSettings->setValue("color", color);
    mLabelSettings->endArray();
    mLabelSettings->sync();
}

void FileLabelModel::addId()
{
    int lastid = lastLabelId();
    mLabelSettings->setValue("lastid", lastid + 1);
    mLabelSettings->sync();
}

void FileLabelModel::initLabelItems()
{
    beginResetModel();
    auto size = mLabelSettings->beginReadArray("labels");
    for (int i = 0; i < size; i++) {
        mLabelSettings->setArrayIndex(i);
        bool visible = mLabelSettings->value("visible").toBool();
        if (visible) {
            auto name = mLabelSettings->value("label").toString();
            auto color = qvariant_cast<QColor>(mLabelSettings->value("color"));

            auto item = new FileLabelItem(this);
            item->mId = i;
            item->setName(name);
            item->setColor(color);

            mLabels.append(item);
        }
    }
    mLabelSettings->endArray();
    endResetModel();
}

FileLabelModel::FileLabelModel(QObject *parent) : QAbstractListModel(parent)
{
    mLabelSettings = new QSettings(QSettings::UserScope, "org.graceful", "fm", this);
    if (mLabelSettings->value("lastid").isNull()) {
        addLabel(tr("Red"), Qt::red);
        addLabel(tr("Orange"), QColor("orange"));
        addLabel(tr("Yellow"), Qt::yellow);
        addLabel(tr("Green"), Qt::green);
        addLabel(tr("Blue"), Qt::blue);
        addLabel(tr("Purple"), QColor("purple"));
        addLabel(tr("Gray"), Qt::gray);
        addLabel(tr("Transparent"), Qt::transparent);
    } else {
        initLabelItems();
    }
}

FileLabelModel::~FileLabelModel()
{

}

FileLabelItem::FileLabelItem(QObject *)
{

}

int FileLabelItem::id()
{
    return mId;
}

const QString FileLabelItem::name()
{
    return mName;
}

const QColor FileLabelItem::color()
{
    return mColor;
}

void FileLabelItem::setName(const QString &name)
{
    mName = name;
    if (mId >= 0) {
        if (gInstance) {
            gInstance->setName(this, name);
        }
    }
}

void FileLabelItem::setColor(const QColor &color)
{
    mColor = color;
    if (mId >= 0) {
        if (gInstance) {
            gInstance->setColor(this, color);
        }
    }
}
