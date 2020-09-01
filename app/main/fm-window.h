#ifndef FMWINDOW_H
#define FMWINDOW_H

#include "fm-window-iface.h"

#include <QTimer>
#include <QSplitter>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolBar>

class TabPage;
class SideBar;
class ToolBar;
class FileInfo;
class SearchBar;
class StatusBar;
class NavigationBar;
class AdvanceSearchBar;
class PreviewPageIface;
class PreviewPageContainer;
class DirectoryViewContainer;
class DirectoryViewProxyIface;

class FMWindow : public QMainWindow, public FMWindowIface
{
    Q_OBJECT
public:
    explicit FMWindow (const QString& uri = nullptr, QWidget *parent = nullptr);

    QSize sizeHint () const override;
    int getCurrentSortColumn () override;
    bool getWindowShowHidden () override;
    const QString getCurrentUri () override;
    FMWindowFactory* getFactory () override;
    bool getWindowSortFolderFirst () override;
    Qt::SortOrder getCurrentSortOrder () override;
    const QString getLastNonSearchUri () override;
    const QString getCurrentPageViewType () override;
    bool getWindowUseDefaultNameSortOrder () override;
    const QStringList getCurrentSelections () override;
    DirectoryViewContainer *getCurrentPage () override;
    const QStringList getCurrentAllFileUris () override;
    FMWindowIface* create (const QString &uri) override;
    FMWindowIface* create (const QStringList &uris) override;
    const QList<std::shared_ptr<FileInfo>> getCurrentSelectionFileInfos () override;


Q_SIGNALS:
    void tabPageChanged ();
    void locationChangeEnd ();
    void locationChangeStart ();
    void windowSelectionChanged ();
    void activeViewChanged(const DirectoryViewProxyIface *view);

public Q_SLOTS:
    void slotRefresh () override;
    void slotClearRecord () override;
    void slotSetShowHidden () override;
    void slotAdvanceSearch () override;
    void slotForceStopLoading () override;
    void slotSetSortFolderFirst () override;
    void slotEditUri (const QString &uri) override;
    void slotSetUseDefaultNameSortOrder () override;
    void slotEditUris (const QStringList &uris) override;
    void slotAddNewTabs (const QStringList &uris) override;
    void slotSetCurrentSortColumn (int sortColumn) override;
    void slotBeginSwitchView (const QString &viewId) override;
    void slotOnPreviewPageSwitch (const QString &uri) override;
    void slotSetCurrentSortOrder (Qt::SortOrder order) override;
    void slotSetCurrentSelectionUris (const QStringList &uris) override;
    void slotFilterUpdate (int type_index=0, int time_index=0, int size_index=0) override;
    void slotGoToUri (const QString &uri, bool addHistory, bool forceUpdate = false) override;
    void slotSearchFilter (QString target_path, QString keyWord, bool search_file_name, bool search_content) override;

protected:
    void keyPressEvent (QKeyEvent *e) override;
    void resizeEvent (QResizeEvent *e) override;

public:
    bool mUpdateCondition = false;

private:
    TabPage *mTab;
    QWidget *mFilter;
    SideBar *mSideBar;
    ToolBar *mToolBar;
    bool mFolderFirst;
    QSplitter *mSplitter;
    bool mShowHiddenFile;
    SearchBar *mSearchBar;
    StatusBar *mStatusBar;
    bool mIsLoading = false;
    QPushButton *mClearRecord;
    QString mAdvanceTargetPath;
    bool mFilterVisible = false;
    QPushButton *mAdvancedButton;
    AdvanceSearchBar *mFilterBar;
    bool mUseDefaultNameSortOrder;
    NavigationBar *mNavigationBar;
    QString mLastNonSearchLocation;
    QTimer mOperationMinimumInterval;
    QStackedWidget *mSideBarContainer = nullptr;
    PreviewPageContainer *mPreviewPageContainer = nullptr;
};

class PreviewPageContainer : public QStackedWidget
{
    friend class FMWindow;
    Q_OBJECT
    explicit PreviewPageContainer(QWidget *parent = nullptr);

    void setCurrentPage(PreviewPageIface *page);
    void removePage(PreviewPageIface *page);
    PreviewPageIface *getCurrentPage();
    bool isHidden() {
        return isVisible();
    }
};

class ToolBarContainer : public QToolBar
{
    friend class FMWindow;
    explicit ToolBarContainer(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e);
};

#endif // FMWINDOW_H
