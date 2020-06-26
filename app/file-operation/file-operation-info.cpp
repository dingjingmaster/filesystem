#include "file-operation-info.h"

#include <QUrl>
#include <file-utils.h>

FileOperationInfo::FileOperationInfo(QStringList srcUris, QString destDirUri, FileOperationInfo::Type type, QObject *parent) : QObject(parent)
{
    mSrcUris = srcUris;
    mDestDirUri = destDirUri;
    mType = type;

    //compute opposite.
    if (type != Rename && type != Link) {
        for (auto srcUri : srcUris) {
            auto srcFile = wrapGFile(g_file_new_for_uri(srcUri.toUtf8().constData()));
            if (mSrcDirUri.isNull()) {
                auto srcParent = FileUtils::getFileParent(srcFile);
                mSrcDirUri = FileUtils::getFileUri(srcParent);
            }
            QString relativePath = FileUtils::getFileBaseName(srcFile);
            auto destDirFile = wrapGFile(g_file_new_for_uri(destDirUri.toUtf8().constData()));
            auto destFile = FileUtils::resolveRelativePath(destDirFile, relativePath);
            QString destUri = FileUtils::getFileUri(destFile);
            mDestUris<<destUri;
        }
    } else {
        if (type == Link) {
            QUrl url = srcUris.first();
            if (!url.fileName().contains(".")) {
                mDestUris<<destDirUri + "/" + url.fileName() + tr(" - Symbolic Link");
            } else {
                auto dest_uri = destDirUri + "/" + url.fileName();
                dest_uri = dest_uri.insert(dest_uri.lastIndexOf('.'), tr(" - Symbolic Link"));
                mDestUris<<dest_uri;
            }
        } else {
            //Rename also use the common args format.
            QString src = srcUris.at(0);
            QString dest = destDirUri;
            mDestUris << src;
            mSrcDirUri = dest;
        }
    }

    switch (type) {
    case Move: {
        mOppositeType = Move;
        break;
    }
    case Trash: {
        mOppositeType = Untrash;
        break;
    }
    case Untrash: {
        mOppositeType = Trash;
        break;
    }
    case Delete: {
        mOppositeType = Other;
        break;
    }
    case Copy: {
        mOppositeType = Delete;
        break;
    }
    case Rename: {
        mOppositeType = Rename;
        break;
    }
    case Link: {
        mOppositeType = Delete;
        break;
    }
    case CreateTxt: {
        mOppositeType = Delete;
        break;
    }
    case CreateFolder: {
        mOppositeType = Delete;
        break;
    }
    case CreateTemplate: {
        mOppositeType = Delete;
        break;
    }
    default: {
        mOppositeType = Other;
    }
    }
}

QString FileOperationInfo::target()
{
    return mDestDirUri;
}

FileOperationInfo::Type FileOperationInfo::operationType()
{
    return mType;
}

QStringList FileOperationInfo::sources()
{
    return mSrcUris;
}

std::shared_ptr<FileOperationInfo> FileOperationInfo::getOppositeInfo(FileOperationInfo& info)
{
    auto oppositeInfo = std::make_shared<FileOperationInfo>(info.mDestUris, info.mSrcDirUri, mOppositeType);
    QMap<QString, QString> oppsiteMap;

    for (auto key : mNodeMap.keys()) {
        auto value = mNodeMap.value(key);
        oppsiteMap.insert(value, key);
    }

    oppositeInfo->mNodeMap = oppsiteMap;
    oppositeInfo->mNewName = this->mOldName;
    oppositeInfo->mOldName = this->mNewName;

    return oppositeInfo;
}
