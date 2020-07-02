#ifndef DESKTOPAPPLICATION_H
#define DESKTOPAPPLICATION_H

#include <single-app/single-app.h>

class QScreen;
class DesktopWindow;
class DesktopIconView;

class DesktopApplication : public SingleApp
{
    Q_OBJECT
public:
    explicit DesktopApplication (int &argc, char *argv[], const char *applicationName = "fm-daemon");

    static DesktopIconView *getIconView ();

protected Q_SLOTS:
    bool slotIsPrimaryScreen (QScreen *screen);
    void slotParseCmd (quint32 id, QByteArray msg, bool isPrimary);

public Q_SLOTS:
    void slotCheckWindowProcess ();
    void slotScreenAddedProcess (QScreen *screen);
    void slotScreenRemovedProcess (QScreen *screen);
    void slotChangeBgProcess (const QString& bgPath);
    void slotPrimaryScreenChangedProcess (QScreen *screen);
    void slotAddWindow (QScreen *screen, bool checkPrimay = true);
    void slotLayoutDirectionChangedProcess (Qt::LayoutDirection direction);

private:
    bool mFirstParse = true;
    QList<DesktopWindow*> mWindowList;
};

#endif // DESKTOPAPPLICATION_H
