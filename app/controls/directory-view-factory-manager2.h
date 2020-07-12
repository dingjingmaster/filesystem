#ifndef DIRECTORYVIEWFACTORYMANAGER2_H
#define DIRECTORYVIEWFACTORYMANAGER2_H

#include <QHash>
#include <QObject>
#include <QSettings>

class GlobalSettings;
class DirectoryViewIface;
class DirectoryViewWidget;
class DirectoryViewPluginIface;
class DirectoryViewPluginIface2;


class DirectoryViewFactoryManager2 : public QObject
{
    Q_OBJECT
public:
    static DirectoryViewFactoryManager2 *getInstance();

    QStringList getFactoryNames();
    DirectoryViewPluginIface2 *getFactory(const QString &name);
    const QString getDefaultViewId(const QString &uri = nullptr);
    const QString getDefaultViewId(int zoomLevel, const QString &uri = nullptr);
    void registerFactory(const QString &name, DirectoryViewPluginIface2 *factory);


    const QStringList internalViews() {return mInternalViews;}

public Q_SLOTS:
    void saveDefaultViewOption();
    void setDefaultViewId(const QString &viewId);

private:
    QHash<QString, DirectoryViewPluginIface2*> *mHash = nullptr;
    explicit DirectoryViewFactoryManager2(QObject *parent = nullptr);
    ~DirectoryViewFactoryManager2();

    GlobalSettings *mSettings;
    QStringList mInternalViews;
    QString mDefaultViewIdCache;
};


#endif // DIRECTORYVIEWFACTORYMANAGER2_H
