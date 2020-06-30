#include "bookmark-manager.h"


BookMarkManager *BookMarkManager::getInstance()
{

}

void BookMarkManager::addBookMark(const QString &uri)
{

}

void BookMarkManager::removeBookMark(const QString &uri)
{

}

BookMarkManager::BookMarkManager(QObject *parent)
{

}

BookMarkManager::~BookMarkManager()
{

}

const QStringList BookMarkManager::getCurrentUris()
{
    return mUris;
}

bool BookMarkManager::isLoaded()
{
    return mIsLoaded;
}
