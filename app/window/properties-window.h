#ifndef PROPERTIESWINDOW_H
#define PROPERTIESWINDOW_H

#include <QMap>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QMainWindow>

class PropertiesWindowTabPagePluginIface;

class PropertiesWindowPluginManager : public QObject
{
    friend class PropertiesWindow;
    Q_OBJECT

public:
    void release ();
    const QStringList getFactoryNames ();
    static PropertiesWindowPluginManager *getInstance ();
    bool registerFactory (PropertiesWindowTabPagePluginIface *factory);
    PropertiesWindowTabPagePluginIface *getFactory (const QString &id);

private:
    ~PropertiesWindowPluginManager () override;
    explicit PropertiesWindowPluginManager (QObject *parent = nullptr);

private:
    QMutex mMutex;
    QMap<int, QString> mSortedFactoryMap;
    QHash<QString, PropertiesWindowTabPagePluginIface*> mFactoryHash;
};

class PropertiesWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit PropertiesWindow(const QStringList &uris, QWidget *parent = nullptr);

    void show ();
    void gotoAboutComputer ();

public:
    QStringList mUris;
};

class PropertiesWindowPrivate : public QTabWidget
{
    friend class PropertiesWindow;
    Q_OBJECT
private:
    explicit PropertiesWindowPrivate (const QStringList &uris, QWidget *parent = nullptr);

};


#endif // PROPERTIESWINDOW_H
