#include "file-item.h"

#include "file-item-model.h"

#include <file/file-info-job.h>
#include <file/file-info-manager.h>

#include <QMessageBox>
#include <QUrl>
#include <file-utils.h>
#include <thumbnail-manager.h>

FileItem::FileItem(std::shared_ptr<FileInfo> info, FileItem *parentItem, FileItemModel *model, QObject *parent) : QObject(parent)
{
    mParent = parentItem;
    mInfo = info;
    mChildren = new QVector<FileItem*>();
    mModel = model;
    mBackendEnumerator = new FileEnumerator(this);

    setParent(mModel);
}

FileItem::~FileItem()
{
    Q_EMIT cancelFindChildren();

    if (mInfo.use_count() <= 2) {
        auto info = FileInfoManager::getInstance()->findFileInfoByUri(mInfo->uri()).get();
        if (info == mInfo.get()) {
            FileInfoManager::getInstance()->lock();
            FileInfoManager::getInstance()->remove(mInfo);
            FileInfoManager::getInstance()->unlock();
        }
    }

    for (auto child : *mChildren) {
        delete child;
    }
    mChildren->clear();

    delete mChildren;
}

bool FileItem::hasChildren()
{
    return mInfo->isDir() || mInfo->isVolume() || mChildren->count() > 0;
}

const QString FileItem::uri()
{
    return mInfo->uri();
}

void FileItem::findChildrenAsync()
{
    if (mExpanded)
        return;

    Q_EMIT mModel->findChildrenStarted();
    mExpanded = true;
    FileEnumerator *enumerator = new FileEnumerator;
    enumerator->setEnumerateDirectory(mInfo->uri());
    //NOTE: entry a new root might destroyed the current enumeration work.
    //the root item will be delete, so we should cancel the previous enumeration.
    enumerator->connect(this, &FileItem::cancelFindChildren, enumerator, &FileEnumerator::slotCancel);
    enumerator->connect(enumerator, &FileEnumerator::prepared, this, [=](std::shared_ptr<GerrorWrapper> err, const QString &targetUri, bool critical) {
        if (critical) {
            QMessageBox::critical(nullptr, tr("Error"), err->message());
            enumerator->slotCancel();
            return;
        }

        if (!targetUri.isNull()) {
            if (targetUri != this->uri()) {
                this->mInfo = FileInfo::fromUri(targetUri);

                GFile *targetFile = g_file_new_for_uri(targetUri.toUtf8().constData());
                auto path = g_file_get_path(targetFile);
                if (path) {
                    QString localUri = QString("file://%1").arg(path);
                    this->mInfo = FileInfo::fromUri(localUri);
                    enumerator->setEnumerateDirectory(localUri);
                    g_free(path);
                } else {
                    enumerator->setEnumerateDirectory(targetFile);
                }

                g_object_unref(targetFile);
            }

            enumerator->slotEnumerateAsync();
            return;
        }

        auto target = FileUtils::getTargetUri(mInfo->uri());
        if (!target.isEmpty()) {
            enumerator->slotCancel();
            //enumerator->deleteLater();
            mModel->setRootUri(target);
            return;
        }
        if (err) {
            if (err.get()->code() == G_IO_ERROR_NOT_FOUND) {
                enumerator->slotCancel();
                //enumerator->deleteLater();
                mModel->setRootUri(FileUtils::getParentUri(this->uri()));
                return;
            } else {
                QMessageBox::critical(nullptr, tr("Error"), err->message());
                enumerator->slotCancel();
                return;
            }
        }
        enumerator->slotEnumerateAsync();
    });

    if (!mModel->isPositiveResponse()) {
        enumerator->connect(enumerator, &FileEnumerator::enumerateFinished, this, [=](bool successed) {
            if (successed) {
                auto infos = enumerator->getChildren(true);
                mAsyncCount = infos.count();
                if (infos.count() == 0) {
                    Q_EMIT mModel->findChildrenFinished();
                }

                for (auto info : infos) {
                    FileItem *child = new FileItem(info, this, mModel);
                    mChildren->prepend(child);
                    FileInfoJob *job = new FileInfoJob(info);
                    job->setAutoDelete();
                    /*
                    FileInfo *shared_info = info.get();
                    int row = infos.indexOf(info);
                    //qDebug()<<info->uri()<<row;
                    job->connect(job, &FileInfoJob::infoUpdated, this, [=](){
                        qDebug()<<shared_info->iconName()<<row;
                    });
                    */
                    connect(job, &FileInfoJob::infoUpdated, this, [=]() {
                        //the query job is finished and will be deleted soon,
                        //whatever info was updated, we need decrease the async count.
                        mAsyncCount--;
                        if (mAsyncCount == 0) {
                            mModel->insertRows(0, mChildren->count(), this->firstColumnIndex());
                            Q_EMIT this->mModel->findChildrenFinished();
                            Q_EMIT mModel->updated();
                            for (auto info : infos) {
                                ThumbnailManager::getInstance()->createThumbnail(info->uri(), mWatcher);
                            }
                        }
                    });
                    job->queryAsync();
                }
            } else {
                Q_EMIT mModel->findChildrenFinished();
                return;
            }

            enumerator->slotCancel();
            delete enumerator;

            mWatcher = std::make_shared<FileWatcher>(this->mInfo->uri());
            mWatcher->setMonitorChildrenChange(true);
            connect(mWatcher.get(), &FileWatcher::fileCreated, this, [=](QString uri) {
                //add new item to mChildren
                //tell the model update
                this->onChildAdded(uri);
                Q_EMIT this->childAdded(uri);
            });
            connect(mWatcher.get(), &FileWatcher::fileDeleted, this, [=](QString uri) {
                //remove the crosponding child
                //tell the model update
                this->onChildRemoved(uri);
                Q_EMIT this->childRemoved(uri);
            });
            connect(mWatcher.get(), &FileWatcher::fileChanged, this, [=](const QString &uri) {
                auto index = mModel->indexFromUri(uri);
                if (index.isValid()) {
                    auto infoJob = new FileInfoJob(FileInfo::fromUri(index.data(FileItemModel::UriRole).toString()));
                    infoJob->setAutoDelete();
                    connect(infoJob, &FileInfoJob::queryAsyncFinished, this, [=]() {
                        mModel->dataChanged(mModel->indexFromUri(uri), mModel->indexFromUri(uri));
                        auto info = FileInfo::fromUri(uri);
                        if (info->isDesktopFile()) {
                            ThumbnailManager::getInstance()->updateDesktopFileThumbnail(info->uri(), mWatcher);
                        }
                    });
                    infoJob->queryAsync();
                }
            });
            connect(mWatcher.get(), &FileWatcher::thumbnailUpdated, this, [=](const QString &uri) {
                mModel->dataChanged(mModel->indexFromUri(uri), mModel->indexFromUri(uri));
            });
            connect(mWatcher.get(), &FileWatcher::directoryDeleted, this, [=](QString uri) {
                //clean all the children, if item index is root index, cd up.
                //this might use FileItemModel::setRootItem()
                Q_EMIT this->deleted(uri);
                this->onDeleted(uri);
            });

            connect(mWatcher.get(), &FileWatcher::locationChanged, this, [=](QString oldUri, QString newUri) {
                //this might use FileItemModel::setRootItem()
                Q_EMIT this->renamed(oldUri, newUri);
                this->onRenamed(oldUri, newUri);
            });

            connect(mWatcher.get(), &FileWatcher::directoryUnmounted, this, [=]() {
                mModel->setRootUri("computer:///");
            });
            //qDebug()<<"startMonitor";
            mWatcher->startMonitor();
        });
    } else {
        enumerator->connect(enumerator, &FileEnumerator::childrenUpdated, this, [=](const QStringList &uris) {
            if (uris.isEmpty()) {
                Q_EMIT mModel->findChildrenFinished();
            }

            if (!mChildren) {
                enumerator->disconnect();
                delete enumerator;
                return ;
            }

            for (auto uri : uris) {
                auto info = FileInfo::fromUri(uri);
                auto item = new FileItem(info, this, mModel);
                mModel->beginInsertRows(firstColumnIndex(), mChildren->count(), mChildren->count());
                mChildren->append(item);
                mModel->endInsertRows();
                auto infoJob = new FileInfoJob(info);
                infoJob->setAutoDelete();
                infoJob->connect(infoJob, &FileInfoJob::infoUpdated, this, [=]() {
                    Q_EMIT mModel->dataChanged(item->firstColumnIndex(), item->lastColumnIndex());
                    //Q_EMIT mModel->updated();
                    ThumbnailManager::getInstance()->createThumbnail(info->uri(), mWatcher);
                });
                infoJob->queryAsync();
            }
        });

        enumerator->connect(enumerator, &FileEnumerator::enumerateFinished, this, [=]() {
            delete enumerator;
            if (!mModel||!mChildren||!mInfo)
                return;

            Q_EMIT mModel->findChildrenFinished();
            Q_EMIT mModel->updated();

            mWatcher = std::make_shared<FileWatcher>(this->mInfo->uri());
            mWatcher->setMonitorChildrenChange(true);
            connect(mWatcher.get(), &FileWatcher::fileCreated, this, [=](QString uri) {
                //add new item to mChildren
                //tell the model update
                this->onChildAdded(uri);
                Q_EMIT this->childAdded(uri);
                ThumbnailManager::getInstance()->createThumbnail(uri, mWatcher);
            });
            connect(mWatcher.get(), &FileWatcher::fileDeleted, this, [=](QString uri) {
                //remove the crosponding child
                //tell the model update
                this->onChildRemoved(uri);
                Q_EMIT this->childRemoved(uri);
            });
            connect(mWatcher.get(), &FileWatcher::fileChanged, this, [=](const QString &uri) {
                auto index = mModel->indexFromUri(uri);
                if (index.isValid()) {
                    auto infoJob = new FileInfoJob(FileInfo::fromUri(index.data(FileItemModel::UriRole).toString()));
                    infoJob->setAutoDelete();
                    connect(infoJob, &FileInfoJob::queryAsyncFinished, this, [=]() {
                        mModel->dataChanged(mModel->indexFromUri(uri), mModel->indexFromUri(uri));
                        auto info = FileInfo::fromUri(uri);
                        if (info->isDesktopFile()) {
                            ThumbnailManager::getInstance()->updateDesktopFileThumbnail(info->uri(), mWatcher);
                        }
                    });
                    infoJob->queryAsync();
                }
            });
            connect(mWatcher.get(), &FileWatcher::thumbnailUpdated, this, [=](const QString &uri) {
                mModel->dataChanged(mModel->indexFromUri(uri), mModel->indexFromUri(uri));
            });
            connect(mWatcher.get(), &FileWatcher::directoryDeleted, this, [=](QString uri) {
                //clean all the children, if item index is root index, cd up.
                //this might use FileItemModel::setRootItem()
                Q_EMIT this->deleted(uri);
                this->onDeleted(uri);
            });
            connect(mWatcher.get(), &FileWatcher::locationChanged, this, [=](QString oldUri, QString newUri) {
                //this might use FileItemModel::setRootItem()
                Q_EMIT this->renamed(oldUri, newUri);
                this->onRenamed(oldUri, newUri);
            });

            connect(mWatcher.get(), &FileWatcher::directoryUnmounted, this, [=]() {
                mModel->setRootUri("computer:///");
            });
            //qDebug()<<"startMonitor";
            mWatcher->startMonitor();
        });
    }

    enumerator->prepare();
}

QModelIndex FileItem::lastColumnIndex()
{
    return mModel->lastColumnIndex(this);
}

QModelIndex FileItem::firstColumnIndex()
{
    return mModel->firstColumnIndex(this);
}

QVector<FileItem *> *FileItem::findChildrenSync()
{
    Q_EMIT mModel->findChildrenStarted();
    std::shared_ptr<FileEnumerator> enumerator = std::make_shared<FileEnumerator>();
    enumerator->setEnumerateDirectory(mInfo->uri());
    enumerator->enumerateSync();
    auto infos = enumerator->getChildren(true);
    for (auto info : infos) {
        FileItem *child = new FileItem(info, this, mModel);
        mChildren->append(child);
        FileInfoJob *job = new FileInfoJob(info);
        job->setAutoDelete();
        job->querySync();
    }
    Q_EMIT mModel->findChildrenFinished();
    return mChildren;
}

const std::shared_ptr<FileInfo> FileItem::info()
{
    return mInfo;
}

bool FileItem::operator ==(const FileItem &item)
{
    return this->mInfo->uri() == item.mInfo->uri();
}

void FileItem::clearChildren()
{
    auto parent = firstColumnIndex();
    mModel->removeRows(0, mModel->rowCount(parent), parent);
    for (auto child : *mChildren) {
        delete child;
    }
    mChildren->clear();
    mExpanded = false;
    mWatcher.reset();
    mWatcher = nullptr;
}

void FileItem::onUpdateDirectoryRequest()
{

}

void FileItem::onChildAdded(const QString &uri)
{
    FileItem *child = getChildFromUri(uri);
    if (child) {
        child->updateInfoSync();
        mModel->updated();
        return;
    }
    FileItem *newChild = new FileItem(FileInfo::fromUri(uri), this, mModel);
    mChildren->append(newChild);
    mModel->insertRow(mChildren->count() - 1, this->firstColumnIndex());
    newChild->updateInfoSync();
    mModel->updated();
}

void FileItem::onDeleted(const QString &thisUri)
{
    if (mParent) {
        if (mParent->mInfo->uri() == thisUri) {
            mModel->removeRow(mParent->mChildren->indexOf(this), mParent->firstColumnIndex());
            mParent->mChildren->removeOne(this);
        } else {
            //if just clear children, there will be a small problem.
            clearChildren();
            mModel->removeRow(mParent->mChildren->indexOf(this), mParent->firstColumnIndex());
            mParent->mChildren->removeOne(this);
            mParent->onChildAdded(mInfo->uri());
        }
        this->deleteLater();
    } else {
        //cd up.
        auto tmpItem = this;
        auto tmpUri = FileUtils::getParentUri(tmpItem->uri());
        while(tmpItem && tmpUri.isNull()) {
            tmpUri = FileUtils::getParentUri(tmpUri);
            tmpItem = tmpItem->mParent;
        }
        if (!tmpUri.isNull()) {
            mModel->setRootUri(tmpUri);
        } else {
            mModel->setRootUri("file:///");
        }
    }
    mModel->updated();
}

void FileItem::onChildRemoved(const QString &uri)
{
    FileItem *child = getChildFromUri(uri);
    if (child) {
        mModel->removeRow(mChildren->indexOf(child), this->firstColumnIndex());
        mChildren->removeOne(child);
    }
    delete child;
    mModel->updated();
}

void FileItem::onRenamed(const QString &oldUri, const QString &newUri)
{
    Q_UNUSED(oldUri);
    if (mParent) {
        FileItem *newRootItem = new FileItem(FileInfo::fromUri(newUri), nullptr, mModel);
        mModel->setRootItem(newRootItem);
    }
}

void FileItem::updateInfoSync()
{
    FileInfoJob *job = new FileInfoJob(mInfo);
    if (job->querySync()) {
        mModel->dataChanged(this->firstColumnIndex(), this->lastColumnIndex());
    }
    job->deleteLater();
}

void FileItem::updateInfoAsync()
{
    FileInfoJob *job = new FileInfoJob(mInfo);
    job->setAutoDelete();
    job->connect(job, &FileInfoJob::infoUpdated, this, [=]() {
        mModel->dataChanged(this->firstColumnIndex(), this->lastColumnIndex());
    });
    job->queryAsync();
}

FileItem *FileItem::getChildFromUri(QString uri)
{
    for (auto item : *mChildren) {
        QUrl url = uri;
        QString decodedUri = url.toDisplayString();
        if (decodedUri == item->uri())
            return item;
    }
    return nullptr;
}
