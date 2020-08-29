#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>

class QComboBox;
class QMenu;

class FMWindow;
class SearchBar;
class ViewFactorySortFilterModel2;

/*!
 * \brief The ToolBar class, providing a set of actions for file management.
 * \details
 * Toolbar is similar to QToolbar, the most great different is that a ToolBar
 * instance can bind with a FMWindow.
 *
 * If ToolBar bound with a FMWindow instance, the signal handler will be passed
 * to the FMWindow, for this case the ToolBar doesn't emit optionRequest signal.
 * Otherwise it would only send the signal.
 */
class ToolBar : public QToolBar
{
    Q_OBJECT
public:
    enum RequestType {
        Ivalid,
        OpenInNewWindow,
        OpenInNewTab,
        OpenInTerminal,
        SwitchView,
        Copy,
        Paste,
        Cut,
        Trash,
        Share,
        Burn,
        Archive,
        Other
    };
    Q_ENUM(RequestType)

    explicit ToolBar(FMWindow *window, QWidget *parent = nullptr);

Q_SIGNALS:
    void optionRequest(const RequestType &type);
    void updateLocationRequest(const QString &uri);

public Q_SLOTS:
    void updateLocation(const QString &uri);
    void updateStates();

protected:
    void init();

private:
    FMWindow *mTopWindow;

    ViewFactorySortFilterModel2 *mViewFactoryModel;
    SearchBar *mSearchBar;

    QList<QAction *> mFileOpActions;
    QAction *mCleanTrashAction;
    QAction *mRestoreAction;

    QAction *mViewAction;
    QMenu *mViewMenu;
    QAction *mSortAction;
    QAction *mTrashActionsSperator;
};

#endif // TOOLBAR_H
