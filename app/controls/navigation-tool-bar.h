#ifndef NAVIGATIONTOOLBAR_H
#define NAVIGATIONTOOLBAR_H
#include <QStack>
#include <QToolBar>

class DirectoryViewContainer;

class NavigationToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit NavigationToolBar(QWidget *parent = nullptr);
    bool canCdUp();
    bool canGoBack();
    bool canGoForward();

Q_SIGNALS:
    void refreshRequest();
    void updateWindowLocationRequest(const QString &uri, bool addHistory, bool forceUpdate = false);

public Q_SLOTS:
    void onGoBack();
    void onGoForward();
    void clearHistory();
    void updateActions();
    void setCurrentContainer(DirectoryViewContainer *container);
    void onGoToUri(const QString &uri, bool addHistory, bool forceUpdate = false);


private:
    QAction *mBackAction;
    QAction *mCdUpAction;
    QAction *mForwardAction;
    QAction *mHistoryAction;
    QAction *mRefreshAction;
    DirectoryViewContainer *mCurrentContainer = nullptr;
};

#endif // NAVIGATIONTOOLBAR_H
