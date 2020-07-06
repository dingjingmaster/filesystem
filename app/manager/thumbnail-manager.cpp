#include "thumbnail-manager.h"

#include <QUrl>
#include <gio/gio.h>
#include <QSemaphore>
#include <QThreadPool>
#include <file-utils.h>
#include <QtConcurrent>
#include <file/file-info.h>
#include <global-settings.h>
#include <file/file-watcher.h>
#include <gio/gdesktopappinfo.h>
#include <thumbnail/pdf-thumbnail.h>
#include <thumbnail/thumbnail-job.h>
#include <thumbnail/generic-thumbnailer.h>

static ThumbnailManager *gInstance = nullptr;

bool ThumbnailManager::hasThumbnail(const QString &uri)
{
    return !mHash.values(uri).isEmpty();
}

ThumbnailManager *ThumbnailManager::getInstance()
{
    if (!gInstance) {
        gInstance = new ThumbnailManager;
    }

    return gInstance;
}

void ThumbnailManager::releaseThumbnail(const QString &uri)
{
    mSemaphore->acquire();
    mHash.remove(uri);
    mSemaphore->release();
}

void ThumbnailManager::setForbidThumbnailInView(bool forbid)
{
    GlobalSettings::getInstance()->setValue("do-not-thumbnail", forbid);
}

const QIcon ThumbnailManager::tryGetThumbnail(const QString &uri)
{
    mSemaphore->acquire();
    auto icon = mHash.value(uri);
    mSemaphore->release();
    //m_mutex.unlock();
    return icon;
}

void ThumbnailManager::updateDesktopFileThumbnail(const QString &uri, std::shared_ptr<FileWatcher> watcher)
{
    auto info = FileInfo::fromUri(uri);
    if (info->isDesktopFile() && info->canExecute()) {
        QtConcurrent::run([=]() {
            QIcon thumbnail;
            QUrl url = uri;
            if (!info->uri().startsWith("file:///")) {
                url = FileUtils::getTargetUri(info->uri());
            }

            auto _desktop_file = g_desktop_app_info_new_from_filename(url.path().toUtf8().constData());
            auto _icon_string = g_desktop_app_info_get_string(_desktop_file, "Icon");
            thumbnail = QIcon::fromTheme(_icon_string);
            QString string = _icon_string;
            if (thumbnail.isNull() && string.startsWith("/")) {
                QIcon thumbnail = GenericThumbnailer::generateThumbnail(_icon_string, true);
            }
            g_free(_icon_string);
            g_object_unref(_desktop_file);

            if (!thumbnail.isNull()) {
                insertOrUpdateThumbnail(uri, thumbnail);
                auto info = FileInfo::fromUri(uri);
                if (watcher) {
                    watcher->thumbnailUpdated(uri);
                }
            }
        });
    } else {
        releaseThumbnail(uri);
        if (watcher) {
            watcher->thumbnailUpdated(uri);
        }
    }
}

void ThumbnailManager::createThumbnail(const QString &uri, std::shared_ptr<FileWatcher> watcher, bool force)
{
    auto thumbnailJob = new ThumbnailJob(uri, watcher, this);
    mThumbnailThreadPool->start(thumbnailJob);
}

void ThumbnailManager::syncThumbnailPreferences()
{
    GlobalSettings::getInstance()->forceSync("do-not-thumbnail");
}

void ThumbnailManager::insertOrUpdateThumbnail(const QString &uri, const QIcon &icon)
{
    mSemaphore->acquire();
    mHash.remove(uri);
    mHash.insert(uri, icon);
    mSemaphore->release();
}

ThumbnailManager::ThumbnailManager(QObject *parent) : QObject(parent)
{
    GlobalSettings::getInstance();

    mThumbnailThreadPool = new QThreadPool(this);
    mThumbnailThreadPool->setMaxThreadCount(1);

    mSemaphore = new QSemaphore(1);
}

ThumbnailManager::~ThumbnailManager()
{
    delete mSemaphore;
}

void ThumbnailManager::createThumbnailInternal(const QString &uri, std::shared_ptr<FileWatcher> watcher, bool force)
{
    auto settings = GlobalSettings::getInstance();
    if (settings->isExist("do-not-thumbnail")) {
        bool do_not_thumbnail = settings->getValue("do-not-thumbnail").toBool();
        if (do_not_thumbnail && !force) {
            return;
        }
    }
    auto info = FileInfo::fromUri(uri);
    if (!info->mimeType().isEmpty()) {
        if (info->mimeType().startsWith("image/")) {
            QUrl url = uri;
            if (!info->uri().startsWith("file:///")) {
                url = FileUtils::getTargetUri(info->uri());
            }
            QIcon thumbnail = GenericThumbnailer::generateThumbnail(url.path(), true);
            if (!thumbnail.isNull()) {

                insertOrUpdateThumbnail(uri, thumbnail);
                auto info = FileInfo::fromUri(uri);
                if (watcher) {
                    watcher->fileChanged(uri);
                }
            }
        } else if (info->mimeType().contains("pdf")) {
            QUrl url = uri;
            if (!info->uri().startsWith("file:///")) {
                url = FileUtils::getTargetUri(info->uri());
            }
            PdfThumbnail pdfThumbnail(info->uri());
            QIcon thumbnail;
            QPixmap pix = pdfThumbnail.generateThumbnail();
            thumbnail = GenericThumbnailer::generateThumbnail(pix, true);
            if (!thumbnail.isNull()) {
                insertOrUpdateThumbnail(uri, thumbnail);
                auto info = FileInfo::fromUri(uri);
                if (watcher) {
                    watcher->fileChanged(uri);
                }
            }
        } else if (info->isDesktopFile()) {
            QIcon thumbnail;
            QUrl url = uri;
            if (!info->uri().startsWith("file:///")) {
                url = FileUtils::getTargetUri(info->uri());
            }

            auto _desktop_file = g_desktop_app_info_new_from_filename(url.path().toUtf8().constData());
            if (!_desktop_file) {
                return;
            }
            auto _icon_string = g_desktop_app_info_get_string(_desktop_file, "Icon");
            thumbnail = QIcon::fromTheme(_icon_string);
            QString string = _icon_string;
            if (thumbnail.isNull() && string.startsWith("/")) {
                QIcon thumbnail = GenericThumbnailer::generateThumbnail(_icon_string, true);
            }
            g_free(_icon_string);
            g_object_unref(_desktop_file);

            if (!thumbnail.isNull()) {
                insertOrUpdateThumbnail(uri, thumbnail);
                auto info = FileInfo::fromUri(uri);
                if (watcher) {
                    watcher->fileChanged(uri);
                }
            }
        }
    }
}
