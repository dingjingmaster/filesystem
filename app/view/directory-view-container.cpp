#include "directory-view-container.h"

DirectoryViewContainer::DirectoryViewContainer(QWidget *parent) : QWidget(parent)
{

}

DirectoryViewContainer::~DirectoryViewContainer()
{

}

bool DirectoryViewContainer::canCdUp()
{

}

bool DirectoryViewContainer::canGoBack()
{

}

bool DirectoryViewContainer::canGoForward()
{

}

Qt::SortOrder DirectoryViewContainer::getSortOrder()
{

}

const QString DirectoryViewContainer::getCurrentUri()
{

}

DirectoryViewWidget *DirectoryViewContainer::getView()
{
    return mView;
}

const QStringList DirectoryViewContainer::getBackList()
{

}

const QStringList DirectoryViewContainer::getAllFileUris()
{

}

const QStringList DirectoryViewContainer::getForwardList()
{

}

FileItemModel::ColumnType DirectoryViewContainer::getSortType()
{

}

const QStringList DirectoryViewContainer::getCurrentSelections()
{

}

void DirectoryViewContainer::cdUp()
{

}

void DirectoryViewContainer::goBack()
{

}

void DirectoryViewContainer::refresh()
{

}

void DirectoryViewContainer::goForward()
{

}

void DirectoryViewContainer::stopLoading()
{

}

void DirectoryViewContainer::updateFilter()
{

}

void DirectoryViewContainer::clearHistory()
{
    mBackList.clear();
    mForwardList.clear();
}

void DirectoryViewContainer::clearConditions()
{

}

void DirectoryViewContainer::tryJump(int index)
{

}

void DirectoryViewContainer::setSortOrder(Qt::SortOrder order)
{

}

void DirectoryViewContainer::setUseDefaultNameSortOrder(bool use)
{

}

void DirectoryViewContainer::setSortFolderFirst(bool folderFirst)
{

}

void DirectoryViewContainer::switchViewType(const QString &viewId)
{

}

void DirectoryViewContainer::setShowHidden(bool showHidden)
{

}

void DirectoryViewContainer::setFilterLabelConditions(QString name)
{

}

void DirectoryViewContainer::onViewDoubleClicked(const QString &uri)
{

}

void DirectoryViewContainer::setSortType(FileItemModel::ColumnType type)
{

}

void DirectoryViewContainer::addFilterCondition(int option, int classify, bool updateNow)
{

}

void DirectoryViewContainer::goToUri(const QString &uri, bool addHistory, bool forceUpdate)
{

}

void DirectoryViewContainer::removeFilterCondition(int option, int classify, bool updateNow)
{

}

void DirectoryViewContainer::setSortFilter(int FileTypeIndex, int FileMTimeIndex, int FileSizeIndex)
{

}

void DirectoryViewContainer::bindNewProxy(DirectoryViewProxyIface *proxy)
{

}
