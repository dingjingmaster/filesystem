#ifndef VIEWFACTORYMODEL_H
#define VIEWFACTORYMODEL_H
#include <QAbstractListModel>

class ViewFactoryModel2 : public QAbstractListModel
{
    friend class ViewFactorySortFilterModel2;
    Q_OBJECT

public:
    explicit ViewFactoryModel2(QObject *parent = nullptr);
    void setDirectoryUri(const QString &uri);

    const QModelIndex getIndexFromViewId(const QString &viewId);

    const QString getViewId(int index);
    const QStringList supportViewIds() {
        return mSupportViewsId;
    }
    const QString getHighestPriorityViewId(int zoom_level_hint);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    QString mCurrentUri;
    QStringList mSupportViewsId;

    QHash<int /*zoom_level*/, QPair<int/*priority*/, QString /*view id*/>> m_factory_hash;
};

#endif // VIEWFACTORYMODEL_H
