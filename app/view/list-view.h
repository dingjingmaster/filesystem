#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "directory-view-widget.h"

#include <QTreeView>

#include <plugin-iface/directory-view-plugin-iface.h>


class FileItemModel;
class FileItemProxyFilterSortModel;

class ListView : public QTreeView, public DirectoryViewIface
{
    friend class ListView2;
    Q_OBJECT
public:
    explicit ListView(QWidget *parent = nullptr);

    const QString viewId() override;
    const QString getDirectoryUri() override;
    const QStringList getSelections() override;
    const QStringList getAllFileUris() override;
    DirectoryViewProxyIface *getProxy() override;
    void setProxy(DirectoryViewProxyIface *proxy) override;
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
    void bindModel(FileItemModel *sourceModel, FileItemProxyFilterSortModel *proxyModel) override;

Q_SIGNALS:
    void zoomLevelChangedRequest(bool zoomIn);

public Q_SLOTS:
    void closeView() override;
    int getSortType() override;
    int getSortOrder() override;
    void invertSelections() override;
    void stopLocationChange() override;
    void beginLocationChange() override;
    void setSortType(int sortType) override;
    void setSortOrder(int sortOrder) override;
    void editUri(const QString &uri) override;
    void editUris(const QStringList uris) override;
    void setDirectoryUri(const QString &uri) override;
    void setCutFiles(const QStringList &uris) override;
    void scrollToSelection(const QString &uri) override;
    void setSelections(const QStringList &uris) override;
    void open(const QStringList &uris, bool newWindow) override;

    void resort();
    void adjustColumnsSize();
    void reportViewDirectoryChanged();

protected:
    void updateGeometries() override;
    void dropEvent(QDropEvent *e) override;
    void wheelEvent (QWheelEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void slotRename();
private:
    bool  mEditValid;
    QTimer* mRenameTimer;
    QModelIndex mLastIndex;
    FileItemModel *mModel = nullptr;
    DirectoryViewProxyIface *mProxy = nullptr;
    FileItemProxyFilterSortModel *mProxyModel = nullptr;

    QString mCurrentUri;

    QSize mLastSize;

    const int BOTTOM_STATUS_MARGIN = 36;
};

class ListView2 : public DirectoryViewWidget
{
    Q_OBJECT
public:
    explicit ListView2(QWidget *parent = nullptr);
    ~ListView2();

    int getSortType();
    bool supportZoom();
    const QString viewId();
    int currentZoomLevel();
    int minimumZoomLevel();
    int maximumZoomLevel();
    Qt::SortOrder getSortOrder();
    const QString getDirectoryUri();
    const QStringList getSelections();
    const QStringList getAllFileUris();


public Q_SLOTS:
    void clearIndexWidget();
    void invertSelections();
    void stopLocationChange();
    void closeDirectoryView();
    void beginLocationChange();
    void setSortType(int sortType);
    void setSortOrder(int sortOrder);
    void editUri(const QString &uri);
    void editUris(const QStringList uris);
    void setCurrentZoomLevel(int zoomLevel);
    void setDirectoryUri(const QString &uri);
    void setCutFiles(const QStringList &uris);
    void scrollToSelection(const QString &uri);
    void setSelections(const QStringList &uris);
    void bindModel(FileItemModel *model, FileItemProxyFilterSortModel *proxyModel);

private:
    ListView *mView = nullptr;
    FileItemModel *mModel = nullptr;
    FileItemProxyFilterSortModel *mProxyModel = nullptr;

    int mZoomLevel = 0;
};

#endif // LISTVIEW_H
