#ifndef ICONVIEW_H
#define ICONVIEW_H

#include <QTimer>
#include <QListView>

#include "directory-view-widget.h"
#include <plugin-iface/directory-view-plugin-iface.h>

class IconViewDelegate;

class IconView : public QListView, public DirectoryViewIface
{
    friend class IconView2;
    friend class IconViewDelegate;
    friend class IconViewIndexWidget;
    Q_OBJECT
public:
    explicit IconView(QWidget *parent = nullptr);
    explicit IconView(DirectoryViewProxyIface *proxy, QWidget *parent = nullptr);
    ~IconView() override;

    bool isDraggingState() {
        return this->state() == QListView::DraggingState || this->state() == QListView::DragSelectingState;
    }

    const QString viewId() override {
        return tr("Icon View");
    }

    void bindModel(FileItemModel *sourceModel, FileItemProxyFilterSortModel *proxyModel) override;
    void setProxy(DirectoryViewProxyIface *proxy) override;

    void setUsePeonyQtDirectoryMenu(bool use);

    DirectoryViewProxyIface *getProxy() override;

    const QString getDirectoryUri() override;

    const QStringList getSelections() override;

    const QStringList getAllFileUris() override;

    QRect visualRect(const QModelIndex &index) const override;

Q_SIGNALS:
    void zoomLevelChangedRequest(bool zoomIn);

public Q_SLOTS:
    void open(const QStringList &uris, bool newWindow) override;
    void setDirectoryUri(const QString &uri) override;
    void beginLocationChange() override;
    void stopLocationChange() override;
    void closeView() override;

    void setSelections(const QStringList &uris) override;
    void invertSelections() override;
    void scrollToSelection(const QString &uri) override;

    void setCutFiles(const QStringList &uris) override;

    int getSortType() override;
    void setSortType(int sortType) override;

    int getSortOrder() override;
    void setSortOrder(int sortOrder) override;

    void editUri(const QString &uri) override;
    void editUris(const QStringList uris) override;

    void resort();
    void reportViewDirectoryChanged();
    void clearIndexWidget();

protected:
    void changeZoomLevel();

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;

    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

    void wheelEvent(QWheelEvent *e) override;

    void updateGeometries() override;

private Q_SLOTS:
    void slotRename();

private:
    QTimer mRepaintTimer;

    bool  mEditValid;
    QTimer* mRenameTimer;

    QModelIndex mLastIndex;

    DirectoryViewProxyIface *mProxy = nullptr;

    FileItemModel * mModel = nullptr;
    FileItemProxyFilterSortModel *mSortFilterProxyModel = nullptr;

    QString mCurrentUri = nullptr;
    bool mUseDirectoryMenu = false;

    const int BOTTOM_STATUS_MARGIN = 36;
};

class IconView2 : public DirectoryViewWidget
{
    Q_OBJECT
public:
    explicit IconView2(QWidget *parent = nullptr);
    ~IconView2();

    const QString viewId() {
        return "Icon View";
    }

    const QString getDirectoryUri() {
        return  mView->getDirectoryUri();
    }

    const QStringList getSelections() {
        return  mView->getSelections();
    }

    const QStringList getAllFileUris() {
        return  mView->getAllFileUris();
    }

    int getSortType() {
        return  mView->getSortType();
    }
    Qt::SortOrder getSortOrder() {
        return Qt::SortOrder( mView->getSortOrder());
    }

    int currentZoomLevel() {
        return  mZoomLevel;
    }
    int minimumZoomLevel() {
        return 21;
    }
    int maximumZoomLevel() {
        return 100;
    }
    bool supportZoom() {
        return true;
    }

public Q_SLOTS:
    void bindModel(FileItemModel *model, FileItemProxyFilterSortModel *proxyModel);

    void setDirectoryUri(const QString &uri) {
         mView->setDirectoryUri(uri);
    }
    void beginLocationChange() {
         mView->beginLocationChange();
    }
    void stopLocationChange() {
         mView->stopLocationChange();
    }

    void closeDirectoryView() {
         mView->closeView();
    }

    void setSelections(const QStringList &uris) {
         mView->setSelections(uris);
    }
    void invertSelections() {
         mView->invertSelections();
    }
    void scrollToSelection(const QString &uri) {
         mView->scrollToSelection(uri);
    }

    void setCutFiles(const QStringList &uris) {
         mView->setCutFiles(uris);
    }

    void setSortType(int sortType) {
         mView->setSortType(sortType);
    }
    void setSortOrder(int sortOrder) {
         mView->setSortOrder(sortOrder);
    }

    void editUri(const QString &uri) {
         mView->editUri(uri);
    }
    void editUris(const QStringList uris) {
         mView->editUris(uris);
    }

    void repaintView();

    void setCurrentZoomLevel(int zoomLevel);

    void clearIndexWidget();

private:
    IconView *mView = nullptr;
    FileItemModel *mModel = nullptr;
    FileItemProxyFilterSortModel *mProxyModel = nullptr;

    int  mZoomLevel = 25;
};

#endif // ICONVIEW_H
