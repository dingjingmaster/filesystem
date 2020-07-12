#include "thumbnail-job.h"

#include <QApplication>
#include <thumbnail-manager.h>
#include <file/file-watcher.h>
#include <syslog/clib_syslog.h>

static int runCount = 0;
static int endCount = 0;

ThumbnailJob::ThumbnailJob(const QString &uri, const std::shared_ptr<FileWatcher> watcher, QObject *parent) : QObject(parent), QRunnable()
{
    mUri = uri;
    mWatcher = watcher;
    if (watcher) {
        if (watcher->parent()) {
            setParent(watcher->parent());
        }
    }
}

ThumbnailJob::~ThumbnailJob()
{
    ++endCount;
}

void ThumbnailJob::run()
{
    if (!parent()) {
        return;
    }

    if (qApp->topLevelWindows().count() == 0) {
        return;
    }

    ++ runCount;

    CT_SYSLOG(LOG_DEBUG, "job start, current end:%d current start request:%d", endCount, runCount);

    setParent(nullptr);
    auto strongPtr = mWatcher.lock();
    ThumbnailManager::getInstance()->createThumbnailInternal(mUri, strongPtr);
}
