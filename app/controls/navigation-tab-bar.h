#ifndef NAVIGATIONTABBAR_H
#define NAVIGATIONTABBAR_H

#include <QTimer>
#include <QTabBar>
#include <QProxyStyle>

class QToolButton;

class NavigationTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit NavigationTabBar(QWidget *parent = nullptr);

Q_SIGNALS:
    void closeWindowRequest();
    void pageAdded(const QString &uri);
    void locationUpdated(const QString &uri);
    void addPageRequest(const QString &uri, bool jumpTo);

public Q_SLOTS:
    void addPages(const QStringList &uri);
    void updateLocation(int index, const QString &uri);
    void addPage(const QString &uri = nullptr, bool jumpToNewTab = false);

protected:
    void tabRemoved(int index) override;
    void tabInserted(int index) override;
    void dropEvent(QDropEvent *e) override;
    void relayoutFloatButton(bool insterted);
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;

private:
    QTimer mDragTimer;
    bool mStartDrag = false;
    QToolButton *mFloatButton;
};

class TabBarStyle : public QProxyStyle
{
    friend class TabWidget;
    friend class NavigationTabBar;
    static TabBarStyle *getStyle();
    TabBarStyle() {}

    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;
};

#endif // NAVIGATIONTABBAR_H
