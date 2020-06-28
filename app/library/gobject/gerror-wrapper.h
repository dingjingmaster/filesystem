#ifndef GERRORWRAPPER_H
#define GERRORWRAPPER_H

#include <memory>
#include <gio/gio.h>
#include <QMetaType>

class GerrorWrapper
{
public:
    GerrorWrapper ();
    GerrorWrapper (GError* err);
    ~GerrorWrapper ();

    int code ();
    QString domain ();
    QString message ();

    static std::shared_ptr<GerrorWrapper> wrapFrom (GError *err);

private:
    GError *mErr = nullptr;
};

typedef std::shared_ptr<GerrorWrapper> GerrorWrapperPtr;

Q_DECLARE_METATYPE(GerrorWrapper)
Q_DECLARE_METATYPE(GerrorWrapperPtr)

#endif // GERRORWRAPPER_H
