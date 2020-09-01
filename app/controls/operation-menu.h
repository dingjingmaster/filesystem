#ifndef OPERATIONMENU_H
#define OPERATIONMENU_H

#include <QMenu>

class MainWindow;
class QToolButton;
class OperationMenuEditWidget;

class OperationMenu : public QMenu
{
    Q_OBJECT
public:
    explicit OperationMenu(MainWindow *window, QWidget *parent = nullptr);

public Q_SLOTS:
    void updateMenu();

private:
    QAction *mShowHidden;
    QAction *mForbidThumbnailing;
    QAction *mResidentInBackend;

private:
    MainWindow *mWindow;
    OperationMenuEditWidget *mEditWidget;
};

class OperationMenuEditWidget : public QWidget
{
public:
    friend class OperationMenu;
    Q_OBJECT

Q_SIGNALS:
    void operationAccepted();

private:
    explicit OperationMenuEditWidget(MainWindow *window, QWidget *parent = nullptr);
    void updateActions(const QString &currentDirUri, const QStringList &selections);

private:
    QToolButton *mCut;
    QToolButton *mCopy;
    QToolButton *mPaste;
    QToolButton *mTrash;
};

#endif // OPERATIONMENU_H
