#ifndef FMDESKTOPAPPLICATION_H
#define FMDESKTOPAPPLICATION_H

#include "desktop-icon-view.h"
#include "desktop-window.h"

#include <single-app/single-app.h>

#include <QScreen>


class FMDesktopApplication : SingleApp
{
    Q_OBJECT
public:
    explicit FMDesktopApplication(int &argc, char *argv[], const char *applicationName = "fm-desktop");

    static DesktopIconView *getIconView ();

protected Q_SLOTS:
    bool isPrimaryScreen (QScreen *screen);
    void parseCmd (quint32 id, QByteArray msg, bool isPrimary);

public Q_SLOTS:
    void checkWindowProcess ();
    void screenAddedProcess (QScreen *screen);
    void screenRemovedProcess (QScreen *screen);
    void changeBgProcess (const QString& bgPath);
    void primaryScreenChangedProcess (QScreen *screen);
    void addWindow (QScreen *screen, bool checkPrimay = true);
    void layoutDirectionChangedProcess (Qt::LayoutDirection direction);

private:
    bool mFirstParse = true;
    QList<DesktopWindow*> mWindowList;
};

#endif // FMDESKTOPAPPLICATION_H
