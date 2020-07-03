#include "desktop-item-proxy-model.h"
#include <syslog/clib_syslog.h>
#include <file/file-info.h>
#include <file/file-meta-info.h>
#include <file-operation-utils.h>

static bool startWithChinese(const QString &displayName);

DesktopItemProxyModel::DesktopItemProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    CT_SYSLOG(LOG_DEBUG, "");
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void DesktopItemProxyModel::setSortType(int type)
{
    mSortType = type;
}

bool DesktopItemProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &source_parent) const
{
    if (!sourceModel()) {
        return false;
    }

    auto sourceIndex = sourceModel()->index(sourceRow, 0, source_parent);
    auto uri = sourceIndex.data(Qt::UserRole).toString();
    auto info = FileInfo::fromUri(uri);
    if (info->displayName().isNull()) {
        return false;
    }
    if (info->displayName().startsWith(".")) {
        return false;
    }
    return true;
}

bool DesktopItemProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    if (mSortType == Other) {
        return true;
    }

    auto leftUri = sourceLeft.data(Qt::UserRole).toString();
    auto leftInfo = FileInfo::fromUri(leftUri);
    auto leftMetaInfo = FileMetaInfo::fromUri(leftUri);

    auto rightUri = sourceRight.data(Qt::UserRole).toString();
    auto rightInfo = FileInfo::fromUri(rightUri);
    auto rightMetaInfo = FileMetaInfo::fromUri(rightUri);

    if (sourceLeft.row() < 3) {
        if (sourceRight.row() < sourceLeft.row()) {
            return (sortOrder()==Qt::AscendingOrder)? false: true;
        }
        return (sortOrder()==Qt::AscendingOrder)? true: false;
    }
    if (sourceRight.row() < 3) {
        if (sourceLeft.row() < sourceRight.row()) {
            return (sortOrder()==Qt::AscendingOrder)? true: false;
        }
        return (sortOrder()==Qt::AscendingOrder)? false: true;
    }

    if (leftInfo->isDir()) {
        if (rightInfo->isDir()) {
        } else {
            return (sortOrder()==Qt::AscendingOrder)? true: false;
        }
    } else {
        if (rightInfo->isDir()) {
            return (sortOrder()==Qt::AscendingOrder)? false: true;
        }
    }

    switch (mSortType) {
    case FileName: {
        if (FileOperationUtils::leftNameIsDuplicatedFileOfRightName(leftInfo->displayName(), rightInfo->displayName())) {
            return FileOperationUtils::leftNameLesserThanRightName(leftInfo->displayName(), rightInfo->displayName());
        }
        if (startWithChinese(leftInfo->displayName())) {
            CT_SYSLOG(LOG_DEBUG, "desktop environment is chinese");
            if (!startWithChinese(rightInfo->displayName())) {
                return (sortOrder()==Qt::AscendingOrder)? true: false;
            } else {
                return !QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
            }
        } else {
            CT_SYSLOG(LOG_DEBUG, "desktop environment is english");
            if (startWithChinese(rightInfo->displayName())) {
                return (sortOrder()==Qt::AscendingOrder)? false: true;
            }
        }
        return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
    }
    case FileType: {
        return leftInfo->type() < rightInfo->type();
    }
    case FileSize: {
        return leftInfo->size() < rightInfo->size();
    }
    case ModifiedDate: {
        return leftInfo->modifiedTime() < rightInfo->modifiedTime();
    }
    }

    return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
}

int DesktopItemProxyModel::getSortType()
{
    return mSortType;
}

bool startWithChinese(const QString &displayName)
{
    //NOTE: a newly created file might could not get display name soon.
    if (displayName.isEmpty()) {
        return false;
    }
    auto firstStrUnicode = displayName.at(0).unicode();
    return (firstStrUnicode <=0x9FA5 && firstStrUnicode >= 0x4E00);
}
