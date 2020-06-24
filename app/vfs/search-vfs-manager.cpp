#include "search-vfs-manager.h"

static SearchVFSManager* globalManager = nullptr;

SearchVFSManager *SearchVFSManager::getInstance()
{
    if (!globalManager) {
        globalManager = new SearchVFSManager;
    }
    return globalManager;
}

void SearchVFSManager::clearHistory()
{
    mMutex.lock();
    mSearchDirResultsHash.clear();
    mMutex.unlock();
}

bool SearchVFSManager::hasHistory(const QString &searchUri)
{
    return mSearchDirResultsHash.contains(searchUri);
}

void SearchVFSManager::clearHistoryOne(const QString &searchUri)
{
    mMutex.lock();
    mSearchDirResultsHash.remove(searchUri);
    mMutex.unlock();
}

QStringList SearchVFSManager::getHistroyResults(const QString &searchUri)
{
    return mSearchDirResultsHash.value(searchUri);
}

void SearchVFSManager::addHistory(const QString &searchUri, const QStringList &results)
{
    mMutex.lock();
    mSearchDirResultsHash.insert(searchUri, results);
    mMutex.unlock();
}

SearchVFSManager::SearchVFSManager(QObject *parent) : QObject(nullptr)
{

}

SearchVFSManager::~SearchVFSManager()
{
    mSearchDirResultsHash.clear();
}
