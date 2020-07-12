#ifndef HEADERBAR_H
#define HEADERBAR_H

#include <QObject>
#include <QToolBar>
#include <QToolButton>
#include <QPushButton>
#include <QProxyStyle>

class HeaderBar;
class MainWindow;
class QHBoxLayout;
class ViewTypeMenu;
class SortTypeMenu;
class OperationMenu;
class AdvancedLocationBar;

class HeaderBarContainer : public QToolBar
{
    Q_OBJECT;
public:
    explicit HeaderBarContainer(QWidget *parent = nullptr);

    bool eventFilter(QObject *obj, QEvent *e);
    void addHeaderBar(HeaderBar *headerBar);

protected:
    void addWindowButtons();

private:
    QHBoxLayout *mLayout;
    QWidget *mInternalWidget;
    HeaderBar *mHeaderBar = nullptr;
    QToolButton *mMaxOrRestore = nullptr;
};

class HeaderBar : public QToolBar
{
    friend class MainWindow;
    friend class HeaderBarContainer;
    Q_OBJECT;
private:
    explicit HeaderBar(MainWindow *parent = nullptr);

Q_SIGNALS:
    void updateSearchRequest(bool showSearch);
    void viewTypeChangeRequest(const QString &viewId);
    void updateZoomLevelHintRequest(int zoomLevelHint);
    void updateLocationRequest(const QString &uri, bool addHistory = true, bool force = true);

protected:
    void addSpacing(int pixel);
    void mouseMoveEvent(QMouseEvent *e);

private Q_SLOTS:
    void finishEdit();
    void closeSearch();
    void updateIcons();
    void updateMaximizeState();
    void searchButtonClicked();
    void openDefaultTerminal();
    void findDefaultTerminal();
    void setSearchMode(bool mode);
    void setLocation(const QString &uri);
    void startEdit(bool bSearch = false);

private:
    const QString mUri;
    MainWindow *mWindow;
    QPushButton *mGoBack;
    QPushButton *mGoForward;
    bool mSearchMode = false;
    QToolButton *mSearchButton;
    ViewTypeMenu *mViewTypeMenu;
    SortTypeMenu *mSortTypeMenu;
    OperationMenu *mOperationMenu;
    AdvancedLocationBar *mLocationBar;
};

class HeaderBarToolButton : public QToolButton
{
    friend class HeaderBar;
    friend class MainWindow;
    Q_OBJECT;
    explicit HeaderBarToolButton(QWidget *parent = nullptr);
};

class HeadBarPushButton : public QPushButton
{
    friend class HeaderBar;
    friend class MainWindow;

    Q_OBJECT
    explicit HeadBarPushButton(QWidget *parent = nullptr);
};

class HeaderBarStyle : public QProxyStyle
{
    friend class HeaderBar;
    friend class HeaderBarContainer;


    HeaderBarStyle();
    static HeaderBarStyle *getStyle();
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
};
#endif // HEADERBAR_H
