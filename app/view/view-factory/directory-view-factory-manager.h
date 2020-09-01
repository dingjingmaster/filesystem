#ifndef DIRECTORYVIEWFACTORYMANAGER_H
#define DIRECTORYVIEWFACTORYMANAGER_H

#include <QHash>
#include <QObject>
#include <QSettings>

class DirectoryViewIface;
class DirectoryViewWidget;
class DirectoryViewPluginIface;
class DirectoryViewPluginIface2;

class DirectoryViewFactoryManager2 : public QObject
{
    Q_OBJECT
public:
    QStringList getFactoryNames();
    static DirectoryViewFactoryManager2 *getInstance();
    DirectoryViewPluginIface2 *getFactory(const QString &name);
    const QString getDefaultViewId(const QString &uri = nullptr);
    const QString getDefaultViewId(int zoomLevel, const QString &uri = nullptr);
    void registerFactory(const QString &name, DirectoryViewPluginIface2 *factory);

public Q_SLOTS:
    void saveDefaultViewOption();
    void setDefaultViewId(const QString &viewId);

private:
    QHash<QString, DirectoryViewPluginIface2*> *mHash = nullptr;
    explicit DirectoryViewFactoryManager2(QObject *parent = nullptr);
    ~DirectoryViewFactoryManager2();

    QSettings *mSettings;
    QStringList mInternalViews;
    QString mDefaultViewIdCache;
};


#endif // DIRECTORYVIEWFACTORYMANAGER_H
