#ifndef DIRECTORYVIEWCONTAINER_H
#define DIRECTORYVIEWCONTAINER_H

#include <QVBoxLayout>
#include <QWidget>

#include <model/file-item-model.h>

class DirectoryViewWidget;
class DirectoryViewProxyIface;
class FileItemProxyFilterSortModel;

class DirectoryViewContainer : public QWidget
{
    Q_OBJECT
public:
    explicit DirectoryViewContainer (QWidget *parent = nullptr);
    ~DirectoryViewContainer ();

    bool canCdUp ();
    bool canGoBack ();
    bool canGoForward ();
    Qt::SortOrder getSortOrder ();
    const QString getCurrentUri ();
    DirectoryViewWidget *getView ();
    const QStringList getBackList ();
    const QStringList getAllFileUris ();
    const QStringList getForwardList ();
    FileItemModel::ColumnType getSortType ();
    const QStringList getCurrentSelections ();

Q_SIGNALS:
    void viewTypeChanged ();
    void directoryChanged ();
    void selectionChanged ();
    void zoomRequest (bool zoomIn);
    void menuRequest (const QPoint &pos);
    void setZoomLevelRequest (int zoomLevel);
    void updateStatusBarSliderStateRequest ();
    void viewDoubleClicked (const QString &uri);
    void updateWindowLocationRequest (const QString &uri, bool addHistory, bool forceUpdate = false);

public Q_SLOTS:
    void cdUp ();
    void goBack ();
    void refresh ();
    void goForward ();
    void stopLoading ();
    void updateFilter ();
    void clearHistory ();
    void clearConditions ();
    void tryJump (int index);
    void setSortOrder (Qt::SortOrder order);
    void setUseDefaultNameSortOrder (bool use);
    void setSortFolderFirst (bool folderFirst);
    void switchViewType (const QString &viewId);
    void setShowHidden (bool showHidden = false);
    void setFilterLabelConditions (QString name);
    void onViewDoubleClicked (const QString &uri);
    void setSortType (FileItemModel::ColumnType type);
    void addFilterCondition (int option, int classify, bool updateNow = false);
    void goToUri (const QString &uri, bool addHistory, bool forceUpdate = false);
    void removeFilterCondition (int option, int classify, bool updateNow = false);
    void setSortFilter (int FileTypeIndex=0, int FileMTimeIndex=0, int FileSizeIndex=0);

protected:
    void bindNewProxy (DirectoryViewProxyIface *proxy);

private:
    QString mCurrentUri;
    QVBoxLayout *mLayout;
    FileItemModel *mModel;
    QStringList mBackList;
    QStringList mForwardList;
    DirectoryViewWidget *mView = nullptr;
    DirectoryViewProxyIface *mProxy = nullptr;
    FileItemProxyFilterSortModel *mProxyModel;
};

#endif // DIRECTORYVIEWCONTAINER_H
