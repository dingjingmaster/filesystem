#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <headerbar.h>
#include <statusbar.h>
#include <QStackedWidget>
#include <navigation-sidebar.h>
#include <border-shadow-effect.h>
#include <controls/tabwidget.h>
#include <main/fm-window-iface.h>

class QWidgetResizeHandler;

/**
 * @brief 文件管理器主要界面布局在此类中完成
 */
class MainWindow : public QMainWindow, public FMWindowIface
{
    Q_OBJECT
public:
    explicit MainWindow (const QString &uri = nullptr, QWidget *parent = nullptr);
    ~MainWindow ();

    QSize sizeHint () const;
    int getCurrentSortColumn ();
    bool getWindowShowHidden ();
    int currentViewZoomLevel ();
    FMWindowFactory *getFactory ();
    const QString getCurrentUri ();
    bool currentViewSupportZoom ();
    bool getWindowSortFolderFirst ();
    Qt::SortOrder getCurrentSortOrder ();
    bool getWindowUseDefaultNameSortOrder ();
    DirectoryViewContainer *getCurrentPage ();
    const QStringList getCurrentSelections ();
    const QStringList getCurrentAllFileUris ();
    FMWindowIface *create (const QString &uri);
    FMWindowIface *create (const QStringList &uris);
    bool eventFilter(QObject *watched, QEvent *event);
    FMWindowIface *createWithZoomLevel (const QString &uri, int zoomLevel);
    const QList<std::shared_ptr<FileInfo>> getCurrentSelectionFileInfos ();
    FMWindowIface *createWithZoomLevel (const QStringList &uris, int zoomLevel);

Q_SIGNALS:
    void locationChangeEnd ();
    void locationChangeStart ();
    void windowSelectionChanged ();
    void viewLoaded (bool successed = true);
    void locationChanged (const QString &uri);

public Q_SLOTS:
    void slotRefresh ();
    void slotCleanTrash ();
    void slotClearRecord ();
    void slotSetShortCuts ();
    void slotAdvanceSearch ();
    void slotSetShowHidden ();
    void slotCheckSettings ();
    void slotUpdateHeaderBar ();
    void slotForceStopLoading ();
    void slotRecoverFromTrash ();
    void slotMaximizeOrRestore ();
    void slotSetSortFolderFirst ();
    void slotUpdateTabPageTitle ();
    void slotCreateFolderOperation ();
    void slotEditUri (const QString &uri);
    void slotSetUseDefaultNameSortOrder ();
    void slotSetLabelNameFilter (QString name);
    void slotEditUris (const QStringList &uris);
    void slotAddNewTabs (const QStringList &uris);
    void slotSetCurrentSortColumn (int sortColumn);
    void slotSetCurrentViewZoomLevel (int zoomLevel);
    void slotBeginSwitchView (const QString &viewId);
    void slotSyncControlsLocation (const QString &uri);
    void slotSetCurrentSortOrder (Qt::SortOrder order);
    void slotSetCurrentSelectionUris (const QStringList &uris);
    void slotFilterUpdate (int type_index=0, int time_index=0, int size_index=0);
    void slotGoToUri (const QString &uri, bool addHistory = false, bool force = false);
    void slotSearchFilter (QString target_path, QString keyWord, bool search_file_name, bool search_content);

protected:
    void validBorder ();
    QRect sideBarRect ();
    void initAdvancePage ();
    void initUI (const QString &uri);
    void paintEvent (QPaintEvent *e);
    void keyPressEvent (QKeyEvent *e);
    void resizeEvent (QResizeEvent *e);
    void mouseMoveEvent (QMouseEvent *e);
    void mousePressEvent (QMouseEvent *e);
    void mouseReleaseEvent (QMouseEvent *e);

private:
    QPoint mOffset;
    TabWidget *mTab;
    bool mFolderFirst;
    bool mShowHiddenFile;
    HeaderBar *mHeaderBar;
    StatusBar *mStatusBar;
    bool mIsSearch = false;
    bool mIsDraging = false;
    BorderShadowEffect *mEffect;
    NavigationSideBar *mSideBar;
    bool mUseDefaultNameSortOrder;
    QWidget *mTransparentAreaWidget;
    QStackedWidget *mSideBarContainer;
    bool mShouldSaveWindowSize = false;
    bool mShouldSaveSideBarWidth = false;
    QWidgetResizeHandler *mResizeHandler;
};

#endif // MAINWINDOW_H
