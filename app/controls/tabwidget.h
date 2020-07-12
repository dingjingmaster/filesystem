#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QList>
#include <QLabel>
#include <memory>
#include <QLineEdit>
#include <QComboBox>
#include <QMainWindow>
#include <QPushButton>
#include <QButtonGroup>
#include <QSignalMapper>
#include <QStackedWidget>
#include <directory-view-container.h>

#include <file/file-info.h>

#include <plugin-iface/preview-page-plugin-iface.h>

class TabWidget : public QMainWindow
{
    friend class MainWindow;
    Q_OBJECT
public:
    explicit TabWidget(QWidget *parent = nullptr);

    bool canCdUp();
    QTabBar *tabBar();
    bool canGoBack();
    int getSortType();
    bool canGoForward();
    Qt::SortOrder getSortOrder();
    const QString getCurrentUri();
    const QStringList getBackList();
    const QStringList getForwardList();
    const QStringList getAllFileUris();
    DirectoryViewContainer *currentPage();
    const QStringList getCurrentSelections();
    bool eventFilter(QObject *obj, QEvent *e);
    const QList<std::shared_ptr<FileInfo>> getCurrentSelectionFileInfos();

Q_SIGNALS:
    void clearTrash();
    void closeSearch();
    void recoverFromTrash();
    void activePageChanged();
    void closeWindowRequest();
    void tabRemoved(int index);
    void tabInserted(int index);
    void zoomRequest(bool zoomIn);
    void currentSelectionChanged();
    void tabMoved(int from, int to);
    void activePageLocationChanged();
    void activePageViewTypeChanged();
    void activePageSelectionChanged();
    void currentIndexChanged(int index);
    void menuRequest(const QPoint &pos);
    void activePageViewSortTypeChanged();
    void viewDoubleClicked(const QString &uri);
    void updateWindowLocationRequest(const QString &uri, bool addHistory, bool forceUpdate = false);

public Q_SLOTS:
    void cdUp();
    void goBack();
    void refresh();
    void goForward();
    void stopLoading();
    void clearHistory();
    void updateFilter();
    void clearConditions();
    void tryJump(int index);
    void updateTabPageTitle();
    void setSortType(int type);
    bool getTriggeredPreviewPage();
    void updateAdvanceConditions();
    void setCurrentIndex(int index);
    void editUri(const QString &uri);
    void editUris(const QStringList &uris);
    void setSortOrder(Qt::SortOrder order);
    void setUseDefaultNameSortOrder(bool use);
    void setSortFolderFirst(bool folderFirst);
    void switchViewType(const QString &viewId);
    void setTriggeredPreviewPage(bool trigger);
    void setShowHidden(bool showHidden = false);
    void onViewDoubleClicked(const QString &uri);
    void setCurrentSelections(const QStringList &uris);
    void addPage(const QString &uri, bool jumpTo = false);
    void setPreviewPage(PreviewPageIface *previewPage = nullptr);
    void addFilterCondition(int option, int classify, bool updateNow = false);
    void goToUri(const QString &uri, bool addHistory, bool forceUpdate = false);
    void setSortFilter(int FileTypeIndex, int FileMTimeIndex, int FileSizeIndex);
    void removeFilterCondition(int option, int classify, bool updateNow = false);

    int count();
    void browsePath();
    int currentIndex();
    void searchUpdate();
    void searchKeyUpdate();
    void updateSearchList();
    void searchChildUpdate();
    void removeTab(int index);
    void addNewConditionBar();
    void removeConditionBar(int index);
    void handleZoomLevel(int zoomLevel);
    void updateSearchBar(bool showSearch);
    void updateTrashBarVisible(const QString &uri = "");
    void updateSearchPathButton(const QString &uri = "");

protected:
    void initAdvanceSearch();
    void updatePreviewPage();
    void updateTabBarGeometry();
    void updateStatusBarGeometry();
    void moveTab(int from, int to);
    void resizeEvent(QResizeEvent *e);
    void changeCurrentIndex(int index);
    QStringList getCurrentClassify(int rowCount);
    void bindContainerSignal(DirectoryViewContainer *container);

private:
    QWidget *mTabBarBg;
    QLabel *mTrashLabel;
    QToolBar *mTrashBar;
    QToolBar *mSearchBar;
    QLabel *mSearchTitle;
    QStackedWidget *mStack;
    QVBoxLayout *mTopLayout;
    QPushButton *mSearchPath;
    QPushButton *mSearchMore;
    QPushButton *mClearButton;
    QPushButton *mSearchClose;
    QPushButton *mSearchChild;
//    NavigationTabBar *mTabBar;
    QPushButton *mRecoverButton;
    QHBoxLayout *mTrashBarLayout;
    QHBoxLayout *mSearchBarLayout;
//    PreviewPageButtonGroups *mButtons;
    QStackedWidget *mPreviewPageContainer;
    PreviewPageIface *mPreviewPage = nullptr;
    QAction *mCurrentPreviewAction = nullptr;

    //use qlist for dynamic generated search conditions list
    int mSearchBarCount = 0;
    QList<QLineEdit*> mInputList;
    QList<QLabel*> mLinkLabelList;
    QList<QHBoxLayout*> mLayoutList;
    QList<QComboBox*> mClassifyList;
    QList<QToolBar*> mSearchBarList;
    QList<QComboBox*> mConditionsList;
    QList<QPushButton*> mAddButtonList;
    QList<QPushButton*> mRemoveButtonList;
    QList<QSignalMapper*> mAddMappperList;
    QList<QSignalMapper*> mRemoveMapperList;


//    TabStatusBar *mStatusBar = nullptr;

    bool mShowSearchBar = false;
    bool mShowSearchList = false;
    bool mSearchChildFlag = false;
    bool mTriggeredPreviewPage = false;

    //Button size macro definition
    QString mLastNonSearchPath = "";
    const int TRASH_BUTTON_WIDTH = 60;
    const int TRASH_BUTTON_HEIGHT = 28;


    //advance search filter options
    QStringList mOptionList = {tr("name"), tr("type"), tr("modify time"), tr("file size")};
    QStringList mFileTypeList = {tr("all"), tr("file folder"), tr("image"), tr("video"), tr("text file"), tr("audio"), tr("others")};
    QStringList mFileMtimeList = {tr("all"), tr("today"), tr("this week"), tr("this month"), tr("this year"), tr("year ago")};
    QStringList mFileSizeList = {tr("all"), tr("tiny(0-16K)"), tr("small(16k-1M)"), tr("medium(1M-100M)"), tr("big(100M-1G)"), tr("large(>1G)")};

};

class PreviewPageContainer : public QStackedWidget
{
    friend class TabWidget;
    Q_OBJECT
    explicit PreviewPageContainer(QWidget *parent = nullptr);
    //QSize sizeHint() const {return QSize(200, QStackedWidget::sizeHint().height());}
};

class PreviewPageButtonGroups : public QButtonGroup
{
    Q_OBJECT
public:
    explicit PreviewPageButtonGroups(QWidget *parent = nullptr);

Q_SIGNALS:
    void previewPageButtonTrigger(bool trigger, const QString &pluginId);
};

#endif // TABWIDGET_H
