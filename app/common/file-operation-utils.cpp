#include "file-operation-utils.h"

#include <file/file-copy-operation.h>
#include <file/file-delete-operation.h>
#include <file/file-enumerator.h>
#include <file/file-info-job.h>
#include <file/file-info.h>
#include <file/file-link-operation.h>
#include <file/file-move-operation.h>
#include <file/file-operation-manager.h>
#include <file/file-rename-operation.h>
#include <file/file-trash-operation.h>
#include <file/file-untrash-operation.h>

#include <QFile>
#include <QMessageBox>
#include <QUrl>

static int getNumOfFileName(const QString &name);

void FileOperationUtils::remove(const QStringList &uris)
{
    auto fileOpMgr = FileOperationManager::getInstance();
    auto removeOp = new FileDeleteOperation(uris);
    fileOpMgr->slotStartOperation(removeOp);
}

void FileOperationUtils::restore(const QString &uriInTrash)
{
    QStringList uris;
    uris<<uriInTrash;
    auto fileOpMgr = FileOperationManager::getInstance();
    auto untrashOp = new FileUntrashOperation(uris);
    fileOpMgr->slotStartOperation(untrashOp, true);
}

void FileOperationUtils::restore(const QStringList &urisInTrash)
{
    auto fileOpMgr = FileOperationManager::getInstance();
    auto untrashOp = new FileUntrashOperation(urisInTrash);
    fileOpMgr->slotStartOperation(untrashOp, false);
}

void FileOperationUtils::trash(const QStringList &uris, bool addHistory)
{
    bool canNotTrash = false;
    for (auto uri : uris) {
        if (!uri.startsWith("file:/")) {
            canNotTrash = true;
        }
    }

    if (!canNotTrash) {
        for (auto uri : uris) {
            QUrl url(uri);
            QFile file(url.path());
            if (file.size() > 1024*1024*1024) {
                canNotTrash = true;
                break;
            }
        }
    }

    if (!canNotTrash) {
        FileEnumerator e;
        e.setEnumerateDirectory("trash:///");
        e.enumerateSync();
        if (e.getChildrenUris().count() > 1000) {
            canNotTrash = true;
        }
    }

    if (canNotTrash) {
        auto result = QMessageBox::question(nullptr, QObject::tr("Can not trash"), QObject::tr("Can not trash these files. "
                                            "You can delete them permanently. "
                                            "Are you sure doing that?"));

        if (result == QMessageBox::Yes) {
            FileOperationUtils::remove(uris);
        }
        return;
    }

    auto fileOpMgr = FileOperationManager::getInstance();
    auto trashOp = new FileTrashOperation(uris);
    fileOpMgr->slotStartOperation(trashOp, addHistory);
}

std::shared_ptr<FileInfo> FileOperationUtils::queryFileInfo(const QString &uri)
{
    auto info = FileInfo::fromUri(uri);
    auto job = new FileInfoJob(info);
    job->querySync();
    job->deleteLater();
    return info;
}

void FileOperationUtils::executeRemoveActionWithDialog(const QStringList &uris)
{
    if (uris.isEmpty())
        return;

    int result = 0;
    if (uris.count() == 1) {
        result = QMessageBox::question(nullptr, QObject::tr("Delete Permanently"), QObject::tr("Are you sure that you want to delete %1? "
                                       "Once you start a deletion, the files deleting will never be "
                                       "restored again.").arg(uris.first().split("/").last()));
    } else {
        result = QMessageBox::question(nullptr, QObject::tr("Delete Permanently"), QObject::tr("Are you sure that you want to delete these %1 files? "
                                       "Once you start a deletion, the files deleting will never be "
                                       "restored again.").arg(uris.count()));
    }

    if (result == QMessageBox::Yes) {
        FileOperationUtils::remove(uris);
    }
}

void FileOperationUtils::rename(const QString &uri, const QString &newName, bool addHistory)
{
    auto fileOpMgr = FileOperationManager::getInstance();
    auto renameOp = new FileRenameOperation(uri, newName);
    fileOpMgr->slotStartOperation(renameOp, addHistory);
}

void FileOperationUtils::link(const QString &srcUri, const QString &destUri, bool addHistory)
{
    auto fileOpMgr = FileOperationManager::getInstance();
    auto linkOp = new FileLinkOperation(srcUri, destUri);
    fileOpMgr->slotStartOperation(linkOp, addHistory);
}

bool FileOperationUtils::leftNameLesserThanRightName(const QString &left, const QString &right)
{
    auto tmpl = left;
    auto tmpr = right;
    int numl = getNumOfFileName(tmpl);
    int numr = getNumOfFileName(tmpr);
    return numl == numr? left < right: numl < numr;
}

void FileOperationUtils::copy(const QStringList &srcUris, const QString &destUri, bool addHistory)
{
    auto fileOpMgr = FileOperationManager::getInstance();
    auto copyOp = new FileCopyOperation(srcUris, destUri);
    fileOpMgr->slotStartOperation(copyOp, addHistory);
}

bool FileOperationUtils::leftNameIsDuplicatedFileOfRightName(const QString &left, const QString &right)
{
    auto tmpl = left;
    auto tmpr = right;
    tmpl.remove(QRegExp("\\(\\d+\\)"));
    tmpr.remove(QRegExp("\\(\\d+\\)"));
    return tmpl == tmpr;
}

void FileOperationUtils::move(const QStringList &srcUris, const QString &destUri, bool addHistory, bool copyMove)
{
    auto fileOpMgr = FileOperationManager::getInstance();
    if (destUri != "trash:///") {
        auto moveOp = new FileMoveOperation(srcUris, destUri);
        moveOp->setCopyMove(copyMove);
        fileOpMgr->slotStartOperation(moveOp, addHistory);
    } else {
        FileOperationUtils::trash(srcUris, true);
    }
}

void FileOperationUtils::create(const QString &destDirUri, const QString &name, FileCreateTemplOperation::Type type)
{
    auto fileOpMgr = FileOperationManager::getInstance();
    auto createOp = new FileCreateTemplOperation(destDirUri, type, name);
    fileOpMgr->slotStartOperation(createOp, true);
}

FileOperationUtils::FileOperationUtils()
{

}

static int getNumOfFileName(const QString &name)
{
    QRegExp regExp("\\(\\d+\\)");
    int num = 0;

    if (name.contains(regExp)) {
        int pos = 0;
        QString tmp;
        while ((pos = regExp.indexIn(name, pos)) != -1) {
            tmp = regExp.cap(0).toUtf8();
            pos += regExp.matchedLength();
        }
        tmp.remove(0,1);
        tmp.chop(1);
        num = tmp.toInt();
    }
    return num;
}
