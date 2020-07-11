#ifndef MAINWINDOWFACTORY_H
#define MAINWINDOWFACTORY_H

#include <main/fm-window-factory.h>



class MainWindowFactory : public FMWindowFactory
{
    Q_OBJECT
public:
    static FMWindowFactory *getInstance();

    FMWindowIface *create(const QString &uri);
    FMWindowIface *create(const QStringList &uris);

private:
    explicit MainWindowFactory(QObject *parent = nullptr);
};

#endif // MAINWINDOWFACTORY_H
