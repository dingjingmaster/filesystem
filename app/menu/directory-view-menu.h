#ifndef DIRECTORYVIEWMENU_H
#define DIRECTORYVIEWMENU_H

#include <QMenu>
#include "plugin-iface/directory-view-plugin-iface2.h"

class FMWindowIface;

class FMWindowIface;

class DirectoryViewMenu : public QMenu
{
    Q_OBJECT
public:
    explicit DirectoryViewMenu(DirectoryViewWidget *directoryView, QWidget *parent = nullptr);
    explicit DirectoryViewMenu(FMWindowIface *window, QWidget *parent = nullptr);

    const QStringList &urisToEdit() {
        return mUrisToEdit;
    }

protected:
    void fillActions();
    const QList<QAction *> constructOpenOpActions();
    const QList<QAction *> constructCreateTemplateActions();
    const QList<QAction *> constructViewOpActions();
    const QList<QAction *> constructFileOpActions();
    const QList<QAction *> constructMenuPluginActions(); //directory view menu extension.
    const QList<QAction *> constructFilePropertiesActions();
    const QList<QAction *> constructComputerActions();
    const QList<QAction *> constructTrashActions();
    const QList<QAction *> constructSearchActions();

private:
    FMWindowIface *mTopWindow;

    DirectoryViewWidget *mView;
    QString mDirectory;
    QStringList mSelections;

    bool mIsComputer = false;
    bool mIsTrash = false;
    bool mIsSearch = false;

    const int ELIDE_TEXT_LENGTH = 16;

    QStringList mUrisToEdit;
};

#endif // DIRECTORYVIEWMENU_H
