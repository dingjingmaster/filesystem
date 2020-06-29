#ifndef FMWINDOWFACTORY_H
#define FMWINDOWFACTORY_H

#include "fm-window-iface.h"

#include <QObject>

class FMWindowFactory : public QObject
{
    Q_OBJECT
public:
    static FMWindowFactory *getInstance();
    virtual FMWindowIface *create (const QStringList &uris);
    virtual FMWindowIface *create (const QString &uri = nullptr);

private:
    explicit FMWindowFactory (QObject *parent = nullptr);

private:
    static FMWindowFactory  *mGlobalInstance;

};

#endif // FMWINDOWFACTORY_H
