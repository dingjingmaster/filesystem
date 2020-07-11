#ifndef FMWINDOWFACTORY_H
#define FMWINDOWFACTORY_H

#include "fm-window-iface.h"

#include <QObject>

class FMWindowFactory : public QObject
{
    Q_OBJECT
public:
    static FMWindowFactory *getInstance();
    explicit FMWindowFactory (QObject *parent = nullptr);
    virtual FMWindowIface *create (const QStringList &uris);
    virtual FMWindowIface *create (const QString &uri = nullptr);

private:

private:
    static FMWindowFactory  *mGlobalInstance;

};

#endif // FMWINDOWFACTORY_H
