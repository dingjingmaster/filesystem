#ifndef NAVIGATIONSIDEBAR_H
#define NAVIGATIONSIDEBAR_H

#include <QTreeView>
#include <QStyledItemDelegate>

class QPushButton;
class QVBoxLayout;
class SideBarModel;
class SideBarProxyFilterSortModel;

class NavigationSideBar : public QTreeView
{
    Q_OBJECT
public:
    QSize sizeHint() const;
    void updateGeometries();
    void resizeEvent(QResizeEvent *e);
    void paintEvent(QPaintEvent *event);
    bool eventFilter(QObject *obj, QEvent *e);
    explicit NavigationSideBar(QWidget *parent = nullptr);
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);


Q_SIGNALS:
    void labelButtonClicked(bool checked);
    void updateWindowLocationRequest(const QString &uri, bool addHistory = true, bool force = false);

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    SideBarModel *mModel;
    SideBarProxyFilterSortModel *mProxyModel;
};

class NavigationSideBarContainer : public QWidget
{
    Q_OBJECT
public:
    explicit NavigationSideBarContainer(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    void addSideBar(NavigationSideBar *sidebar);

private:
    QPushButton *mLabelButton;
    QVBoxLayout *mLayout = nullptr;
    NavigationSideBar *mSidebar = nullptr;
};

class NavigationSideBarItemDelegate : public QStyledItemDelegate
{
    friend class NavigationSideBar;
    explicit NavigationSideBarItemDelegate(QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // NAVIGATIONSIDEBAR_H
