#ifndef FMDBUSSERVICE_H
#define FMDBUSSERVICE_H

#include <QObject>


class FMDBusService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.FileManager1")

public:
    explicit FMDBusService (QObject *parent = nullptr);
    Q_SCRIPTABLE void ShowItems (const QStringList& uriList, const QString& startUpId);
    Q_SCRIPTABLE void ShowFolders (const QStringList& uriList, const QString& startUpId);
    Q_SCRIPTABLE void ShowItemProperties (const QStringList& uriList, const QString& startUpId);

Q_SIGNALS:
    void showItemsRequest (const QStringList& uriList, const QString& startUpId);
    void showFolderRequest (const QStringList& uriList, const QString& startUpId);
    void showItemPropertiesRequest (const QStringList& uriList, const QString& startUpId);
};
#endif // FMDBUSSERVICE_H
