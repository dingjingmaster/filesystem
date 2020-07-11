#ifndef DESKTOPMENU_H
#define DESKTOPMENU_H

#include <QMenu>

#include <plugin-iface/directory-view-plugin-iface.h>


class DirectoryViewIface;

class DesktopMenu : public QMenu
{
    Q_OBJECT
public:
    explicit DesktopMenu(DirectoryViewIface *view, QWidget *parent = nullptr);
    const QStringList urisToEdit ();

protected:
    void fillActions();
    const QList<QAction *> constructOpenOpActions();
    const QList<QAction *> constructViewOpActions();
    const QList<QAction *> constructFileOpActions();
    const QList<QAction *> constructMenuPluginActions();
    const QList<QAction *> constructCreateTemplateActions();
    const QList<QAction *> constructFilePropertiesActions();

    void gotoAboutComputer();
    void openWindow(const QString &uri);
    void showProperties(const QString &uri);
    void openWindow(const QStringList &uris);
    void showProperties(const QStringList &uris);

private:
    QString mDirectory;
    QStringList mSelections;
    QStringList mUrisToEdit;
    DirectoryViewIface *mView;

    const int ELIDE_TEXT_LENGTH = 16;
};
#endif // DESKTOPMENU_H
