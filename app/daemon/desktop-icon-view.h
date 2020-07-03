#ifndef DESKTOPICONVIEW_H
#define DESKTOPICONVIEW_H

#include "desktop-item-model.h"
#include "desktop-item-proxy-model.h"

#include <QTimer>
#include <QListView>

#include <plugin-iface/directory-view-plugin-iface.h>


class DesktopIconView : public QListView, public DirectoryViewIface
{
    friend class DesktopIndexWidget;
    friend class DesktopIconViewDelegate;
    Q_OBJECT
public:
    enum ZoomLevel
    {
        Invalid,
        Small,          // icon: 24x24; grid: 64x64; hover rect: 60x60; font: system*0.8
        Normal,         // icon: 48x48; grid: 96x96; hover rect = 90x90; font: system
        Large,          // icon: 64x64; grid: 115x135; hover rect = 105x118; font: system*1.2
        Huge            // icon: 96x96; grid: 140x170; hover rect = 120x140; font: system*1.4

    };
    Q_ENUM(ZoomLevel)

    explicit DesktopIconView(QWidget *parent = nullptr);
    ~DesktopIconView();

    void initMenu();
    int getSortType ();
    int getSortOrder ();
    void initShoutCut();
    void initDoubleClick();
    const QString viewId ();
    const QString getDirectoryUri ();
    const QStringList getSelections ();
    const QStringList getAllFileUris ();
    bool eventFilter (QObject *obj, QEvent *e);
    void setProxy (DirectoryViewProxyIface *proxy);
    QRect visualRect (const QModelIndex &index) const;
    const QFont getViewItemFont (QStyleOptionViewItem *item);
    void bindModel (FileItemModel *sourceModel, FileItemProxyFilterSortModel *proxyModel);

Q_SIGNALS:
    void zoomLevelChanged (ZoomLevel level);

public Q_SLOTS:
    void zoomIn();
    void zoomOut();
    void refresh();
    void closeView();
    void invertSelections();
    void stopLocationChange();
    void beginLocationChange();
    void clearAllIndexWidgets();
    ZoomLevel zoomLevel() const;
    void setSortType(int sortType);
    void setSortOrder(int sortOrder);
    void editUri(const QString &uri);
    void saveAllItemPosistionInfos();
    void resetAllItemPositionInfos();
    DirectoryViewProxyIface *getProxy ();
    void editUris(const QStringList uris);
    void setDirectoryUri(const QString &uri);
    void setDefaultZoomLevel(ZoomLevel level);
    void setCutFiles(const QStringList &uris);
    void scrollToSelection(const QString &uri);
    void setSelections(const QStringList &uris);
    void saveItemPositionInfo(const QString &uri);
    void resetItemPosistionInfo(const QString &uri);
    void open(const QStringList &uris, bool newWindow);
    void updateItemPosistions(const QString &uri = nullptr);

protected:
    bool isItemsOverlapped();
    void dropEvent(QDropEvent *e);
    void wheelEvent(QWheelEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void resizeEvent(QResizeEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void dragEnterEvent(QDragEnterEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *event);

private:
    QTimer mRefreshTimer;
    QModelIndex mLastIndex;
    QTimer mEditTriggerTimer;
    bool mRealDoEdit = false;
    bool mIsRefreshing = false;
    QModelIndexList mDragIndexes;
    ZoomLevel mZoomLevel = Invalid;
    bool mCtrlOrShiftPressed = false;
    QStringList mNewFilesToBeSelected;
    DesktopItemProxyModel *mProxyModel;
    DesktopItemModel *mModel = nullptr;
};

#endif // DESKTOPICONVIEW_H
