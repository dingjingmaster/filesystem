#include "view-factory-model.h"
#include <QPair>

#include "plugin-iface/directory-view-plugin-iface.h"
#include "plugin-iface/directory-view-plugin-iface2.h"
#include "view/view-factory/directory-view-factory-manager.h"

//ViewFactory2 model
ViewFactoryModel2::ViewFactoryModel2(QObject *parent)
    : QAbstractListModel(parent)
{

}

void ViewFactoryModel2::setDirectoryUri(const QString &uri)
{
    beginResetModel();
    mSupportViewsId.clear();
    m_factory_hash.clear();
    mCurrentUri = uri;
    auto viewManager = DirectoryViewFactoryManager2::getInstance();
    auto defaultList = viewManager->getFactoryNames();

    for (auto id : defaultList) {
        if (viewManager->getFactory(id)->supportUri(mCurrentUri)) {
            QPair<int, QString> pair(viewManager->getFactory(id)->priority(mCurrentUri), id);
            m_factory_hash.insert(viewManager->getFactory(id)->zoom_level_hint(),
                                  pair);

            mSupportViewsId<<id;
        }
    }

    endResetModel();
}

const QString ViewFactoryModel2::getViewId(int index)
{
    if (index > mSupportViewsId.count() - 1 || index < 0) {
        return nullptr;
    }
    return mSupportViewsId.at(index);
}

int ViewFactoryModel2::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return mSupportViewsId.count();
}

const QString ViewFactoryModel2::getHighestPriorityViewId(int zoom_level_hint)
{
    auto manager = DirectoryViewFactoryManager2::getInstance();
    auto pairs = m_factory_hash.values(zoom_level_hint);
    DirectoryViewPluginIface2 *factory = nullptr;
    int priority = -9999;
    QString result;
    for (auto pair : pairs) {
        factory = manager->getFactory(pair.second);
        if (factory->priority(mCurrentUri) > priority) {
            result = pair.second;
            priority = pair.first;
        }
    }
    return result;
}

QVariant ViewFactoryModel2::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto viewManager = DirectoryViewFactoryManager2::getInstance();
    auto factory = viewManager->getFactory(viewManager->getFactoryNames().at(index.row()));

    switch (role) {
    case Qt::DecorationRole:
        return factory->viewIcon();
    case Qt::ToolTipRole:
        return factory->viewIdentity();
    case Qt::UserRole:
        return factory->viewIdentity();
    default:
        break;
    }
    return QVariant();
}

const QModelIndex ViewFactoryModel2::getIndexFromViewId(const QString &viewId)
{
    if (!mSupportViewsId.contains(viewId))
        return QModelIndex();
    return index(mSupportViewsId.indexOf(viewId));
}
