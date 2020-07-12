#include "sidebar-filesystem-item.h"

#include "sidebar-model.h"
#include "sidebar-separator-item.h"

#include <password.h>
#include <file-utils.h>
#include <file/file-info.h>
#include <QAbstractItemModel>
#include <file/file-watcher.h>
#include <syslog/clib_syslog.h>
#include <file/file-info-job.h>
#include <file/file-enumerator.h>
#include <window/volume-manager.h>

static GAsyncReadyCallback eject_cb(GFile *file, GAsyncResult *res, SideBarFileSystemItem *pThis);

SideBarFileSystemItem::SideBarFileSystemItem(QString uri, SideBarFileSystemItem *parentItem, SideBarModel *model, QObject *parent) : SideBarAbstractItem (model, parent)
{
    mParent = parentItem;

    if (parentItem == nullptr) {
        mIsRootChild = true;
        mUri = "computer:///";
        mDisplayName = tr("Computer");
        mIconName = "";
    } else {
        mUri = uri;
        mIconName = FileUtils::getFileIconName(uri);
        mDisplayName = FileUtils::getFileDisplayName(uri);
        FileUtils::queryVolumeInfo(mUri, mVolumeName, mUnixDevice, mDisplayName);
    }
}

SideBarAbstractItem::Type SideBarFileSystemItem::type()
{
    return SideBarAbstractItem::FileSystemItem;
}

QString SideBarFileSystemItem::uri()
{
    return mUri;
}

bool SideBarFileSystemItem::isMounted()
{
    return mIsMounted;
}

QString SideBarFileSystemItem::iconName()
{
    return mIconName;
}

bool SideBarFileSystemItem::hasChildren()
{
    return true;
}

bool SideBarFileSystemItem::isEjectable()
{
    if (mUri.contains("computer:///") && mUri != "computer:///") {
        auto info = FileInfo::fromUri(mUri);
        if (info->displayName().isEmpty()) {
            FileInfoJob j(info);
            j.querySync();
        }
        return info->canEject();
    }
    return false;
}

bool SideBarFileSystemItem::isMountable()
{
    if (mUri.contains("computer:///") && mUri != "computer:///") {
        auto info = FileInfo::fromUri(mUri);
        if (info->displayName().isEmpty()) {
            FileInfoJob j(info);
            j.querySync();
        }
        return info->canMount() || info->canUnmount();
    }
    return false;
}

bool SideBarFileSystemItem::isRemoveable()
{
    if (mUri.contains("computer:///") && mUri != "computer:///") {
        auto info = FileInfo::fromUri(mUri);
        if (info->displayName().isEmpty()) {
            FileInfoJob j (info);
            j.querySync();
        }
        return info->canEject() || info->canStop();
    }
    return false;
}

QString SideBarFileSystemItem::displayName()
{
    QString displayName;
    if (!mVolumeName.isEmpty()) {
        displayName += mVolumeName;
    }
    if (!mUnixDevice.isEmpty()) {
        displayName += QString(" (%1)").arg(mUnixDevice);
    }
    if (!displayName.isEmpty())
        return displayName;

    return mDisplayName;
}

QModelIndex SideBarFileSystemItem::lastColumnIndex()
{
    return mModel->lastCloumnIndex(this);
}

SideBarAbstractItem *SideBarFileSystemItem::parent()
{
    return mParent;
}

QModelIndex SideBarFileSystemItem::firstColumnIndex()
{
    return mModel->firstCloumnIndex(this);
}

void SideBarFileSystemItem::eject()
{
    auto file = wrapGFile(g_file_new_for_uri(this->uri().toUtf8().constData()));
    auto target = FileUtils::getTargetUri(mUri);
    auto drive = VolumeManager::getDriveFromUri(target);
    g_file_eject_mountable_with_operation(file.get()->get(), G_MOUNT_UNMOUNT_NONE, nullptr, nullptr, GAsyncReadyCallback(eject_cb), this);
}

void SideBarFileSystemItem::format()
{
}

void SideBarFileSystemItem::unmount()
{
    auto file = wrapGFile(g_file_new_for_uri(this->uri().toUtf8().constData()));
    g_file_unmount_mountable_with_operation(file.get()->get(), G_MOUNT_UNMOUNT_NONE, nullptr, nullptr, nullptr, nullptr);
}

void SideBarFileSystemItem::onUpdated()
{

}

void SideBarFileSystemItem::findChildren()
{
    auto pwdItem = Password::getCurrentUser();
    if (pwdItem.userId() == 0) {
        return;
    }
    clearChildren();

    FileEnumerator *e = new FileEnumerator;
    e->setEnumerateDirectory(mUri);
    connect(e, &FileEnumerator::prepared, this, [=](const GerrorWrapperPtr &err, const QString &targetUri) {
        if (targetUri != nullptr) {
            if (targetUri != this->uri()) {
                e->setEnumerateDirectory(targetUri);
            }
        }

        e->enumerateSync();
        auto infos = e->getChildren();
        bool isEmpty = true;
        int real_children_count = infos.count();
        if (infos.isEmpty()) {
            auto separator = new SideBarSeparatorItem(SideBarSeparatorItem::EmptyFile, this, mModel);
            this->mChildren->prepend(separator);
            mModel->insertRows(0, 1, this->firstColumnIndex());
            goto end;
        }

        for (auto info: infos) {
            if (!info->displayName().startsWith(".") && (info->isDir() || info->isVolume())) {
                isEmpty = false;
            }
            if (!(info->isDir() || info->isVolume())) {
                real_children_count--;
                continue;
            }

            SideBarFileSystemItem *item = new SideBarFileSystemItem(info->uri(), this, mModel, this);
            auto targetUri = FileUtils::getTargetUri(info->uri());
            bool isUmountable = FileUtils::isFileUnmountable(info->uri());
            item->mIsMounted = (!targetUri.isEmpty() && (targetUri != "file:///")) || isUmountable;
            mChildren->append(item);
        }
        mModel->insertRows(0, real_children_count, firstColumnIndex());

        if (isEmpty) {
            auto separator = new SideBarSeparatorItem(SideBarSeparatorItem::EmptyFile, this, mModel);
            this->mChildren->prepend(separator);
            mModel->insertRows(0, 1, this->firstColumnIndex());
        }
end:
        Q_EMIT this->findChildrenFinished();
        if (err != nullptr) {
        }
        delete e;

        this->initWatcher();
        this->mWatcher->setMonitorChildrenChange();
        connect(mWatcher.get(), &FileWatcher::fileCreated, this, [=](const QString &uri) {
            for (auto item : *mChildren) {
                if (item->uri() == uri)
                    return;
            }

            SideBarFileSystemItem *item = new SideBarFileSystemItem(uri,
                    this,
                    mModel);
            mChildren->append(item);
            mModel->insertRows(mChildren->count() - 1, 1, firstColumnIndex());
            mModel->indexUpdated(this->firstColumnIndex());
        });

        connect(mWatcher.get(), &FileWatcher::fileDeleted, this, [=](const QString &uri) {
            for (auto child : *mChildren) {
                if (child->uri() == uri) {
                    int index = mChildren->indexOf(child);
                    mModel->removeRows(index, 1, firstColumnIndex());
                    mChildren->removeOne(child);
                    child->deleteLater();
                    break;
                }
            }
            mModel->indexUpdated(this->firstColumnIndex());
        });

        connect(mWatcher.get(), &FileWatcher::fileChanged, this, [=](const QString &uri) {
            for (auto child : *mChildren) {
                if (child->uri() == uri) {
                    SideBarFileSystemItem *changedItem = static_cast<SideBarFileSystemItem*>(child);
                    if (FileUtils::getTargetUri(uri).isEmpty()) {
                        changedItem->mIsMounted = false;
                        changedItem->clearChildren();
                    } else {
                        changedItem->mIsMounted = true;
                    }

                    mModel->dataChanged(changedItem->firstColumnIndex(), changedItem->firstColumnIndex());
                    break;
                }
            }
        });

        this->startWatcher();
    });
    e->prepare();

    Q_EMIT findChildrenFinished();
}

void SideBarFileSystemItem::clearChildren()
{
    stopWatcher();
    SideBarAbstractItem::clearChildren();
}

void SideBarFileSystemItem::findChildrenAsync()
{
    findChildren();
}

void SideBarFileSystemItem::initWatcher()
{
    if (!mWatcher) {
        mWatcher = std::make_shared<FileWatcher>(mUri);
    }
}

void SideBarFileSystemItem::stopWatcher()
{
    initWatcher();
    mWatcher->stopMonitor();
}

void SideBarFileSystemItem::startWatcher()
{
    initWatcher();
    mWatcher->startMonitor();
}

static GAsyncReadyCallback eject_cb(GFile *file, GAsyncResult *res, SideBarFileSystemItem *pThis)
{
    GError *err = nullptr;
    bool successed = g_file_eject_mountable_with_operation_finish(file, res, &err);
    if (err) {
        CT_SYSLOG(LOG_ERR, "%s", err->message);
        g_error_free(err);
    }
    return nullptr;
}
