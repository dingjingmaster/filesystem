#include "sidebar-personal-item.h"

#include <file-utils.h>
#include <QStandardPaths>
#include "sidebar-model.h"


SideBarPersonalItem::SideBarPersonalItem(QString uri, SideBarPersonalItem *parentItem, SideBarModel *model, QObject *parent) : SideBarAbstractItem (model, parent)
{
    mParent = parentItem;
    mIsRootChild = parentItem == nullptr;
    if (mIsRootChild) {
        QString homeUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        mUri = homeUri;
        mDisplayName = tr("Personal");
        mIconName = "";

        QString documentUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        SideBarPersonalItem *documentItem = new SideBarPersonalItem(documentUri, this, mModel);
        mChildren->append(documentItem);

        QString pictureUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        SideBarPersonalItem *pictureItem = new SideBarPersonalItem(pictureUri, this, mModel);
        mChildren->append(pictureItem);

        QString mediaUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        SideBarPersonalItem *mediaItem = new SideBarPersonalItem(mediaUri, this, mModel);
        mChildren->append(mediaItem);

        QString downloadUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        SideBarPersonalItem *downloadItem = new SideBarPersonalItem(downloadUri, this, mModel);
        mChildren->append(downloadItem);

        QString musicUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        SideBarPersonalItem *musicItem = new SideBarPersonalItem(musicUri, this, mModel);
        mChildren->append(musicItem);

        mModel->insertRows(0, 5, firstColumnIndex());

        return;
    }
    mUri = uri;
    mDisplayName = FileUtils::getFileDisplayName(uri);
    mIconName = FileUtils::getFileIconName(uri);
}

SideBarAbstractItem::Type SideBarPersonalItem::type()
{
    return SideBarAbstractItem::PersonalItem;
}

QString SideBarPersonalItem::uri()
{
    return mUri;
}

QString SideBarPersonalItem::displayName()
{
    return mDisplayName;
}

QModelIndex SideBarPersonalItem::lastColumnIndex()
{
    return mModel->lastCloumnIndex(this);
}

SideBarAbstractItem *SideBarPersonalItem::parent()
{
    return mParent;
}

QModelIndex SideBarPersonalItem::firstColumnIndex()
{
    return mModel->firstCloumnIndex(this);
}

void SideBarPersonalItem::eject()
{

}

void SideBarPersonalItem::format()
{

}

void SideBarPersonalItem::unmount()
{

}

void SideBarPersonalItem::onUpdated()
{

}

void SideBarPersonalItem::findChildren()
{

}

void SideBarPersonalItem::clearChildren()
{

}

void SideBarPersonalItem::findChildrenAsync()
{

}

bool SideBarPersonalItem::isRemoveable()
{
    return false;
}

bool SideBarPersonalItem::isEjectable()
{
    return false;
}

bool SideBarPersonalItem::isMountable()
{
    return false;
}

QString SideBarPersonalItem::iconName()
{
    return mIconName;
}

bool SideBarPersonalItem::hasChildren()
{
    return mIsRootChild;
}
