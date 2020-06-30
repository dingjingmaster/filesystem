#ifndef MENUPLUGINIFACE_H
#define MENUPLUGINIFACE_H

#include <QAction>
#include <QObject>

#define MenuPluginIface_iid "org.graceful.fm.plugin-iface.MenuPluginIface"

class MenuPluginIface : public QObject
{
    Q_OBJECT
public:
    enum Type
    {
        Invalid,
        SideBar,
        DesktopWindow,
        DirectoryView,
        Other
    };
    Q_DECLARE_FLAGS(Types, Type)

    explicit MenuPluginIface (QObject *parent = nullptr);
    virtual ~MenuPluginIface ();


    virtual QString testPlugin () = 0;
    virtual QList<QAction*> menuActions (Types types, const QString &uri, const QStringList &selectionUris) = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MenuPluginIface::Types)
Q_DECLARE_INTERFACE (MenuPluginIface, MenuPluginIface_iid)

#endif // MENUPLUGINIFACE_H
