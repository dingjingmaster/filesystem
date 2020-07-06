#include "clipbord-utils.h"
#include "file-utils.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>

#include <file/file-copy-operation.h>
#include <file/file-move-operation.h>
#include <file/file-operation-manager.h>

static ClipbordUtils *gInstance = nullptr;
static QString gClipboardParentUri = nullptr;

ClipbordUtils *ClipbordUtils::getInstance()
{
    if (!gInstance) {
        gInstance = new ClipbordUtils;
    }
    return gInstance;
}

void ClipbordUtils::release()
{
    delete gInstance;
}

void ClipbordUtils::clearClipboard()
{
    QApplication::clipboard()->clear();
}

bool ClipbordUtils::isClipboardHasFiles()
{
    return QApplication::clipboard()->mimeData()->hasUrls();
}

bool ClipbordUtils::isClipboardFilesBeCut()
{
    if (isClipboardHasFiles()) {
        auto data = QApplication::clipboard()->mimeData();
        if (data->hasFormat("graceful-filesystem/is-cut")) {
            QVariant var(data->data("graceful-filesystem/is-cut"));
            return var.toBool();
        }
    }
    return false;
}

QStringList ClipbordUtils::getClipboardFilesUris()
{
    QStringList l;

    if (!isClipboardHasFiles()) {
        return l;
    }
    auto urls = QApplication::clipboard()->mimeData()->urls();
    for (auto url : urls) {
        l << url.toString();
    }
    return l;
}

const QString ClipbordUtils::getClipedFilesParentUri()
{
    return gClipboardParentUri;
}

void ClipbordUtils::pasteClipboardFiles(const QString &targetDirUri)
{
    if (!isClipboardHasFiles()) {
        return;
    }

    auto uris = getClipboardFilesUris();
    for (auto uri : getClipboardFilesUris()) {
        if (!FileUtils::isFileExsit(uri)) {
            uris.removeAll(uri);
        }
    }
    if (uris.isEmpty()) {
        return;
    }

    auto fileOpMgr = FileOperationManager::getInstance();
    if (isClipboardFilesBeCut()) {
        auto moveOp = new FileMoveOperation(uris, targetDirUri);
        fileOpMgr->slotStartOperation(moveOp, true);
        QApplication::clipboard()->clear();
    } else {
        auto copyOp = new FileCopyOperation(uris, targetDirUri);
        fileOpMgr->slotStartOperation(copyOp, true);
    }
}

void ClipbordUtils::setClipboardFiles(const QStringList &uris, bool isCut)
{
    if (!gInstance) {
        gInstance = new ClipbordUtils;
    }

    gClipboardParentUri = FileUtils::getParentUri(uris.first());

    auto data = new QMimeData;
    QVariant isCutData = QVariant(isCut);
    data->setData("graceful-filesystem/is-cut", isCutData.toByteArray());
    QList<QUrl> urls;
    for (auto uri : uris) {
        urls << uri;
    }
    data->setUrls(urls);
    QApplication::clipboard()->setMimeData(data);
}

ClipbordUtils::ClipbordUtils(QObject *parent) : QObject(parent)
{
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &ClipbordUtils::clipboardChanged);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, [=]() {
        auto data = QApplication::clipboard()->mimeData();
        if (!data->hasFormat("graceful-filesystem/is-cut")) {
            gClipboardParentUri = nullptr;
        }
    });
}

ClipbordUtils::~ClipbordUtils()
{

}
