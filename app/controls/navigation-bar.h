#ifndef NAVIGATIONBAR_H
#define NAVIGATIONBAR_H

#include <QToolBar>

class QMenu;
class QActionGroup;
class NavigationToolBar;
class AdvancedLocationBar;
class DirectoryViewContainer;

class NavigationBar : public QToolBar
{
    Q_OBJECT
public:
    explicit NavigationBar(QWidget *parent = nullptr);
    ~NavigationBar();

    bool isPathEditing();
    const QString getLastPreviewPageId();

Q_SIGNALS:
    void refreshRequest();
    void switchPreviewPageRequest(const QString &id);
    void updateWindowLocationRequest(const QString &uri, bool addHistory, bool forceUpdate = false);

public Q_SLOTS:
    void startEdit();
    void finishEdit();
    void setBlock(bool block = true);
    void triggerAction(const QString &id);
    void updateLocation(const QString &uri);
    void bindContainer(DirectoryViewContainer *container);

protected:
    const QString getCurrentUri();
    void paintEvent(QPaintEvent *e);

private:
    QActionGroup *mGroup;
    QAction *mCheckedPreviewAction;
    NavigationToolBar *mLeftControl;
    AdvancedLocationBar *mCenterControl;
    QString mLastPreviewPageIdInWindow = nullptr;
};

#endif // NAVIGATIONBAR_H
