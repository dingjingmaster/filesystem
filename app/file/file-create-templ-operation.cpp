#include "file-create-templ-operation.h"

#include <utime.h>
#include <gio/gio.h>
#include <QMessageBox>

#define TEMPLATE_DIR "file://" + QString(g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES)) + "/"

FileCreateTemplOperation::FileCreateTemplOperation(const QString &destDirUri, FileCreateTemplOperation::Type type, const QString &templateName, QObject *parent) : FileOperation(parent)
{
    mTargetUri = destDirUri + "/" + templateName;
    QStringList srcUris;
    mSrcUri = TEMPLATE_DIR +templateName;
    mDestDirUri = destDirUri;
    mType = type;
    mInfo = std::make_shared<FileOperationInfo>(srcUris, destDirUri, FileOperationInfo::Type::Copy);
}

void FileCreateTemplOperation::run()
{
    Q_EMIT operationStarted();
    Q_EMIT operationPrepared();
    switch (mType) {
    case EmptyFile: {
        mTargetUri = mDestDirUri + "/" + tr("NewFile");
retry_create_empty_file:
        GError *err = nullptr;
        g_file_create(wrapGFile(g_file_new_for_uri(mTargetUri.toUtf8())).get()->get(), G_FILE_CREATE_NONE, nullptr, &err);
        if (err) {
            if (err->code == G_IO_ERROR_EXISTS) {
                g_error_free(err);
                handleDuplicate(mTargetUri);
                goto retry_create_empty_file;
            } else {
                Q_EMIT errored(mSrcUri, mDestDirUri, GerrorWrapper::wrapFrom(err), true);
            }
        }
        break;
    }
    case EmptyFolder: {
        mTargetUri = mDestDirUri + "/" + tr("NewFolder");
retry_create_empty_folder:
        GError *err = nullptr;
        g_file_make_directory(wrapGFile(g_file_new_for_uri(mTargetUri.toUtf8())).get()->get(), nullptr, &err);
        if (err) {
            if (err->code == G_IO_ERROR_EXISTS) {
                g_error_free(err);
                handleDuplicate(mTargetUri);
                goto retry_create_empty_folder;
            } else {
                Q_EMIT errored(mSrcUri, mDestDirUri, GerrorWrapper::wrapFrom(err), true);
            }
        }
        break;
    }
    case Template: {
retry_create_template:
        GError *err = nullptr;
        g_file_copy(wrapGFile(g_file_new_for_uri(mSrcUri.toUtf8())).get()->get(), wrapGFile(g_file_new_for_uri(mTargetUri.toUtf8())).get()->get(), GFileCopyFlags(G_FILE_COPY_NOFOLLOW_SYMLINKS), nullptr, nullptr, nullptr, &err);
        if (err) {
            if (err->code == G_IO_ERROR_EXISTS) {
                g_error_free(err);
                handleDuplicate(mTargetUri);
                goto retry_create_template;
            } else {
                Q_EMIT errored(mSrcUri, mDestDirUri, GerrorWrapper::wrapFrom(err), true);
            }
        }
        // change file's modify time and access time after copy templete file;
        time_t now_time = time(NULL);
        struct utimbuf tm = {now_time, now_time};

        if (mTargetUri.startsWith("file://")) {
            utime (mTargetUri.toStdString().c_str() + 6, &tm);
        } else if (mTargetUri.startsWith("/")) {
            utime (mTargetUri.toStdString().c_str(), &tm);
        }

        break;
    }
    }
    Q_EMIT operationFinished();
    notifyFileWatcherOperationFinished();
}

std::shared_ptr<FileOperationInfo> FileCreateTemplOperation::getOperationInfo()
{
    return mInfo;
}

void FileCreateTemplOperation::handleDuplicate(const QString &uri)
{
    setHasError(true);
    QString name = uri.split("/").last();
    QRegExp regExp("\\(\\d+\\)");
    if (name.contains(regExp)) {
        int pos = 0;
        int num = 0;
        QString tmp;
        while ((pos = regExp.indexIn(name, pos)) != -1) {
            tmp = regExp.cap(0).toUtf8();
            pos += regExp.matchedLength();
        }
        tmp.remove(0,1);
        tmp.chop(1);
        num = tmp.toInt();

        num++;
        name = name.replace(regExp, QString("(%1)").arg(num));
        mTargetUri = mDestDirUri + "/" + name;
    } else {
        if (name.contains(".")) {
            auto list = name.split(".");
            if (list.count() <= 1) {
                mTargetUri = mDestDirUri + "/" + name + "(1)";
            } else {
                int pos = list.count() - 1;
                if (list.last() == "gz" |
                        list.last() == "xz" |
                        list.last() == "Z" |
                        list.last() == "sit" |
                        list.last() == "bz" |
                        list.last() == "bz2") {
                    pos--;
                }
                if (pos < 0)
                    pos = 0;
                //list.insert(pos, "(1)");
                auto tmp = list;
                QStringList suffixList;
                for (int i = 0; i < list.count() - pos; i++) {
                    suffixList.prepend(tmp.takeLast());
                }
                auto suffix = suffixList.join(".");

                auto basename = tmp.join(".");
                name = basename + "(1)" + "." + suffix;
                if (name.endsWith("."))
                    name.chop(1);
                mTargetUri = mDestDirUri + "/" + name;
            }
        } else {
            name = name + "(1)";
            mTargetUri = mDestDirUri + "/" + name;
        }
    }
}

const QString FileCreateTemplOperation::target()
{
    return mTargetUri;
}
