#ifndef DESKTOPITEMMODEL_H
#define DESKTOPITEMMODEL_H

#include <memory>
#include <QAbstractListModel>
#include <QQueue>

class FileInfo;
class FileWatcher;
class FileEnumerator;

class DesktopItemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role
    {
        UriRole = Qt::UserRole,
        IsLinkRole = Qt::UserRole + 1
    };
    Q_ENUM(Role)

    explicit DesktopItemModel(QObject *parent = nullptr);
    ~DesktopItemModel() override;

    const QString indexUri(const QModelIndex &index);
    const QModelIndex indexFromUri(const QString &uri);
    Qt::DropActions supportedDropActions() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QMimeData *mimeData(const QModelIndexList& indexes) const override;
    bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    bool insertRow(int row, const QModelIndex &parent = QModelIndex());
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

Q_SIGNALS:
    void refreshed();
    void requestClearIndexWidget();
    void fileCreated(const QString &uri);
    void requestLayoutNewItem(const QString &uri);
    void requestUpdateItemPositions(const QString &uri = nullptr);

public Q_SLOTS:
    void refresh();

protected Q_SLOTS:
    void onEnumerateFinished();

private:
    FileEnumerator *mEnumerator;
    QQueue<QString> mInfoQueryQueue;
    QList<std::shared_ptr<FileInfo>> mFiles;
    std::shared_ptr<FileWatcher> mTrashWatcher;
    std::shared_ptr<FileWatcher> mDesktopWatcher;
    std::shared_ptr<FileWatcher> mThumbnailWatcher;
};

#endif // DESKTOPITEMMODEL_H
