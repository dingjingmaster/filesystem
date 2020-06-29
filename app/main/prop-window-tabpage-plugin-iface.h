#ifndef PROPWINDOWTABPAGEPLUGINIFACE_H
#define PROPWINDOWTABPAGEPLUGINIFACE_H

#include <QObject>

class PropWindowTabpagePluginIface : public QObject
{
    friend class PropWindowTabpagePluginIface;
    Q_OBJECT
public:
    explicit PropWindowTabpagePluginIface(QObject *parent = nullptr);

Q_SIGNALS:

};

#endif // PROPWINDOWTABPAGEPLUGINIFACE_H
