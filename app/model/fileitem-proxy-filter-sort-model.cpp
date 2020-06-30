#include "file-item-model.h"
#include "fileitem-proxy-filter-sort-model.h"
#include "filelabel-model.h"

#include <QLocale>
#include <QCollator>
#include <file-utils.h>
#include "file-item.h"
#include <global-settings.h>
#include <file-operation-utils.h>
#include <QTime>

QLocale locale = QLocale(QLocale::system().name());
QCollator comparer = QCollator(locale);

FileItemProxyFilterSortModel::FileItemProxyFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    auto settings = GlobalSettings::getInstance();
    mShowHidden = settings->isExist("show-hidden")? settings->getValue("show-hidden").toBool(): false;
    mUseDefaultNameSortOrder = settings->isExist("chinese-first")? settings->getValue("chinese-first").toBool(): false;
    mFolderFirst = settings->isExist("folder-first")? settings->getValue("folder-first").toBool(): true;
}

void FileItemProxyFilterSortModel::clearConditions()
{
    mFileTypeList.clear();
    mFileSizeList.clear();
    mModifyTimeList.clear();
}

QStringList FileItemProxyFilterSortModel::getAllFileUris()
{
    QStringList l;
    auto indexes = getAllFileIndexes();
    for (auto index : indexes) {
        if (index.column() == 0)
            l<<index.data(FileItemModel::UriRole).toString();
    }
    return l;
}

QModelIndexList FileItemProxyFilterSortModel::getAllFileIndexes()
{
    QModelIndexList l;
    int i = 0;
    while (this->index(i, 0, QModelIndex()).isValid()) {
        auto index = this->index(i, 0, QModelIndex());
        if (mShowHidden) {
            l<<index;
        } else {
            auto disyplayName = index.data(Qt::DisplayRole).toString();
            if (disyplayName.isEmpty()) {
                auto uri = this->index(i, 0, QModelIndex()).data(FileItemModel::UriRole).toString();
                disyplayName = FileUtils::getFileDisplayName(uri);
            }
            if (!disyplayName.startsWith(".")) {
                l<<index;
            }
        }

        i++;
    }
    return l;
}

void FileItemProxyFilterSortModel::setShowHidden(bool showHidden)
{
    GlobalSettings::getInstance()->setValue("show-hidden", showHidden);
    mShowHidden = showHidden;
    invalidateFilter();
}

void FileItemProxyFilterSortModel::setFolderFirst(bool folderFirst)
{
    GlobalSettings::getInstance()->setValue("folder-first", folderFirst);
    mFolderFirst = folderFirst;
    beginResetModel();
    sort(sortColumn()>0? sortColumn(): 0, sortOrder()==Qt::DescendingOrder? Qt::DescendingOrder: Qt::AscendingOrder);
    endResetModel();
}

void FileItemProxyFilterSortModel::setUseDefaultNameSortOrder(bool use)
{
    GlobalSettings::getInstance()->setValue("chinese-first", use);
    mUseDefaultNameSortOrder = use;
    beginResetModel();
    sort(sortColumn()>0? sortColumn(): 0, sortOrder()==Qt::DescendingOrder? Qt::DescendingOrder: Qt::AscendingOrder);
    endResetModel();
}

const QModelIndex FileItemProxyFilterSortModel::indexFromUri(const QString &uri)
{
    FileItemModel *model = static_cast<FileItemModel*>(sourceModel());
    const QModelIndex sourceIndex = model->indexFromUri(uri);
    return mapFromSource(sourceIndex);
}

FileItem *FileItemProxyFilterSortModel::itemFromIndex(const QModelIndex &proxyIndex)
{
    FileItemModel *model = static_cast<FileItemModel*>(sourceModel());
    QModelIndex index = mapToSource(proxyIndex);
    return model->itemFromIndex(index);
}

void FileItemProxyFilterSortModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel()) {
        disconnect(sourceModel());
    }

    QSortFilterProxyModel::setSourceModel(model);
    FileItemModel *file_item_model = static_cast<FileItemModel*>(model);
    connect(file_item_model, &FileItemModel::updated, this, &FileItemProxyFilterSortModel::update);
}

QModelIndex FileItemProxyFilterSortModel::getSourceIndex(const QModelIndex &proxyIndex)
{
    return mapToSource(proxyIndex);
}

void FileItemProxyFilterSortModel::setMutipleLabelConditions(QStringList names, QList<QColor> colors)
{
    mShowLabelNames.clear();
    mShowLabelColors.clear();

    for(auto name : names) {
        mShowLabelNames.append(name);
    }
    for(auto color : colors) {
        mShowLabelColors.append(color);
    }
    invalidateFilter();
}

void FileItemProxyFilterSortModel::setLabelBlurName(QString blurName, bool caseSensitive)
{
    mBlurName = blurName;
    mCaseSensitive = caseSensitive;
    invalidateFilter();
}

void FileItemProxyFilterSortModel::addFilterCondition(int option, int classify, bool updateNow)
{
    switch (option) {
    case 1:
        if (! mFileTypeList.contains(classify)) {
            mFileTypeList.append(classify);
        }

        break;
    case 2:
        if (! mModifyTimeList.contains(classify)) {
            mModifyTimeList.append(classify);
        }
        break;
    case 3:
        if (! mFileSizeList.contains(classify)) {
            mFileSizeList.append(classify);
        }
        break;
    default:
        break;
    }

    if (updateNow) {
        invalidateFilter();
    }
}

void FileItemProxyFilterSortModel::setFilterConditions(int fileType, int modifyTime, int fileSize)
{
    mShowFileType = fileType;
    mShowFileSize = fileSize;
    mShowModifyTime = modifyTime;
    invalidateFilter();
}

void FileItemProxyFilterSortModel::removeFilterCondition(int option, int classify, bool updateNow)
{
    switch (option) {
    case 1:
        if (! mFileTypeList.contains(classify)) {
            mFileTypeList.removeOne(classify);
        }
        break;
    case 2:
        if (! mModifyTimeList.contains(classify)) {
            mModifyTimeList.removeOne(classify);
        }
        break;
    case 3:
        if (! mFileSizeList.contains(classify)) {
            mFileSizeList.removeOne(classify);
        }
        break;
    default:
        break;
    }

    if (updateNow)
        invalidateFilter();
}

void FileItemProxyFilterSortModel::setFilterLabelConditions(QString name, QColor color)
{
    mLabelName = name;
    mLabelColor = color;
    invalidateFilter();
}

void FileItemProxyFilterSortModel::update()
{
    invalidateFilter();
}

bool FileItemProxyFilterSortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (left.isValid() && right.isValid()) {
        FileItemModel *model = static_cast<FileItemModel*>(sourceModel());
        auto leftItem = model->itemFromIndex(left);
        auto rightItem = model->itemFromIndex(right);
        if (!(leftItem->hasChildren() && rightItem->hasChildren())) {
            //make folder always has a higher order.
            if (!leftItem->hasChildren() && !rightItem->hasChildren()) {
                goto default_sort;
            }
            if (mFolderFirst) {
                bool lesser = leftItem->hasChildren();
                if (sortOrder() == Qt::AscendingOrder)
                    return lesser;
                return !lesser;
            }
        }

default_sort:
        switch (sortColumn()) {
        case FileItemModel::FileName: {
            if (FileOperationUtils::leftNameIsDuplicatedFileOfRightName(leftItem->mInfo->displayName(), rightItem->mInfo->displayName())) {
                return FileOperationUtils::leftNameLesserThanRightName(leftItem->info()->displayName(), rightItem->info()->displayName());
            }
            if (mUseDefaultNameSortOrder) {
                QString leftDisplayName = leftItem->mInfo->displayName();
                QString rightDisplayName = rightItem->mInfo->displayName();
                bool leftStartWithChinese = startWithChinese(leftDisplayName);
                bool rightStartWithChinese = startWithChinese(rightDisplayName);

                if (leftStartWithChinese && rightStartWithChinese) {
                    return comparer.compare(leftDisplayName, rightDisplayName) < 0;
                }

                if (leftStartWithChinese || rightStartWithChinese) {
                    if (sortOrder() == Qt::AscendingOrder) {
                        return leftStartWithChinese;
                    }
                    return rightStartWithChinese;
                }
            }
            return leftItem->mInfo->displayName().toLower() < rightItem->mInfo->displayName().toLower();
        }
        case FileItemModel::FileSize: {
            return leftItem->mInfo->size() < rightItem->mInfo->size();
        }
        case FileItemModel::FileType: {
            return leftItem->mInfo->fileType() < rightItem->mInfo->fileType();
        }
        case FileItemModel::ModifiedDate: {
            return leftItem->mInfo->modifiedTime() < rightItem->mInfo->modifiedTime();
        }
        default:
            break;
        }
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

bool FileItemProxyFilterSortModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    FileItemModel *model = static_cast<FileItemModel*>(sourceModel());
    auto childIndex = model->index(sourceRow, 0, sourceParent);
    if (childIndex.isValid()) {
        auto item = static_cast<FileItem*>(childIndex.internalPointer());
        if (!mShowHidden) {
            if (item->mInfo->displayName() != nullptr) {
                if (item->mInfo->displayName().at(0) == '.') {
                    return false;
                }
            }
        }

        if (! checkFileTypeFilter(item->mInfo->type())) {
            return false;
        }

        if (! checkFileModifyTimeFilter(item->mInfo->modifiedTime())) {
            return false;
        }

        if (! checkFileSizeFilter(item->mInfo->size())) {
            return false;
        }

        if (mLabelName != "" || mLabelColor != Qt::transparent) {
            QString uri = item->mInfo->uri();
            if (mLabelName != "") {
                auto names = FileLabelModel::getGlobalModel()->getFileLabels(uri);
                if (! names.contains(mLabelName)) {
                    return false;
                }
            }

            if (mLabelColor != Qt::transparent) {
                auto colors = FileLabelModel::getGlobalModel()->getFileColors(uri);
                if (! colors.contains(mLabelColor)) {
                    return false;
                }
            }
        }

        if(mShowLabelNames.size() >0 || mShowLabelColors.size() >0) {
            bool bfind = false;
            QString uri = item->mInfo->uri();
            if (mShowLabelNames.size() >0 ) {
                auto names = FileLabelModel::getGlobalModel()->getFileLabels(uri);
                for(auto temp : mShowLabelNames) {
                    if(names.contains(temp)) {
                        bfind = true;
                        break;
                    }
                }
            }

            if (! bfind && mShowLabelColors.size() >0) {
                auto colors = FileLabelModel::getGlobalModel()->getFileColors(uri);
                for(auto temp : mShowLabelColors) {
                    if (colors.contains(temp)) {
                        bfind = true;
                        break;
                    }
                }
            }

            if (! bfind) {
                return false;
            }
        }

        if (mBlurName != "") {
            QString uri = item->mInfo->uri();
            auto names = FileLabelModel::getGlobalModel()->getFileLabels(uri);
            bool find = false;
            for(auto temp : names) {
                if ((mCaseSensitive && temp.indexOf(mBlurName) >= 0) || (! mCaseSensitive && temp.toLower().indexOf(mBlurName.toLower()) >= 0)) {
                    find = true;
                    break;
                }
            }
            if (! find) {
                return false;
            }
        }
    }
    return true;
}

bool FileItemProxyFilterSortModel::checkFileTypeFilter(QString type) const
{
    if ((mShowFileType == ALL_FILE && mFileTypeList.count() == 0) || mFileTypeList.contains(ALL_FILE)) {
        return true;
    }

    QList<int> totalTypeList;
    if (mFileTypeList.count() > 0) {
        totalTypeList.append(mFileTypeList);
    }

    if (! totalTypeList.contains(mShowFileType) && mShowFileType != ALL_FILE) {
        totalTypeList.append(mShowFileType);
    }

    for(int i=0; i<totalTypeList.count(); i++) {
        auto cur = totalTypeList[i];
        switch (cur) {
        case FILE_FOLDER: {
            if (type == cFolderType)
                return true;
            break;
        }
        case PICTURE: {
            if (type.contains(cImageType))
                return true;
            break;
        }
        case VIDEO: {
            if (type.contains(cVideoType))
                return true;
            break;
        }
        case TXT_FILE: {
            if (type.contains(cTextType))
                return true;
            break;
        }
        case AUDIO: {
            if (type.contains(cAudioType))
                return true;
            break;
        }
        case OTHERS: {
            if (type != cFolderType && ! type.contains(cImageType) && ! type.contains(cVideoType) && ! type.contains(cTextType) && ! type.contains(cAudioType))
                return true;
            break;
        }
        default:
            break;
        }
    }

    return false;
}

bool FileItemProxyFilterSortModel::checkFileSizeFilter(quint64 size) const
{
    if (mFileSizeList.count() == 0 || mFileSizeList.contains(ALL_FILE)) {
        return true;
    }

    for(int i=0; i < mFileSizeList.count(); i++) {
        auto cur = mFileSizeList[i];
        switch (cur) {
        case TINY: {    // [0-16K)
            if (size < 16 * K_BASE)
                return true;
            break;
        }
        case SMALL: {   // [16k-1M]
            if(size >= 16 * K_BASE && size <= K_BASE * K_BASE) {
                return true;
            }
            break;
        }
        case MEDIUM: {  // (1M-100M]
            if(size > K_BASE * K_BASE && size <= 100 * K_BASE * K_BASE) {
                return true;
            }
            break;
        }
        case BIG: {     // (100M-1G]
            if(size > 100 * K_BASE * K_BASE && size <= K_BASE * K_BASE * K_BASE) {
                return true;
            }
            break;
        }
        case LARGE: {   // >1G
            if (size > K_BASE * K_BASE * K_BASE)
                return true;
            break;
        }
        default:
            break;
        }
    }

    return false;
}

bool FileItemProxyFilterSortModel::startWithChinese(const QString &displayName) const
{
    if (displayName.isEmpty()) {
        return false;
    }
    auto firstStrUnicode = displayName.at(0).unicode();
    return (firstStrUnicode <=0x9FA5 && firstStrUnicode >= 0x4E00);
}

bool FileItemProxyFilterSortModel::checkFileModifyTimeFilter(quint64 modifiedTime) const
{
    if (mModifyTimeList.count() == 0 || mModifyTimeList.contains(ALL_FILE)) {
        return true;
    }

    auto time = QDateTime::currentMSecsSinceEpoch();
    auto dateTime = QDateTime::fromMSecsSinceEpoch(time);
    QDate date = dateTime.date();
    int year = date.year();
    int month = date.month();
    int day = date.day();
    QDate mdDate =  QDateTime::fromMSecsSinceEpoch(modifiedTime * 1000).date();
    int mdDay = mdDate.day();
    int mdYear= mdDate.year();
    int mdMonth = mdDate.month();

    for(int i=0; i< mModifyTimeList.size(); i++) {
        auto cur = mModifyTimeList[i];
        switch(cur) {
        case TODAY: {
            if (year == mdYear && month == mdMonth && day == mdDay) {
                return true;
            }
            break;
        }
        case THIS_WEEK: {
            QDate md_date(mdYear, mdMonth, mdDay);
            int week, mdWeek,weekYear, mdWeekYear;
            week = date.weekNumber(&weekYear);
            mdWeek = md_date.weekNumber(&mdWeekYear);
            if (week == mdWeek && weekYear == mdWeekYear) {
                return true;
            }
            break;
        }
        case THIS_MONTH: {
            if (year == mdYear && month == mdMonth) {
                return true;
            }
            break;
        }
        case THIS_YEAR: {
            if (year == mdYear) {
                return true;
            }
            break;
        }
        case YEAR_AGO: {
            if(year > mdYear) {
                return true;
            }
            break;
        }
        default:
            break;
        }
    }

    return false;
}
