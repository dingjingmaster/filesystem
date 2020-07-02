#include "fm-dbus-service.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>

FMDBusService::FMDBusService(QObject *parent) : QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/freedesktop/FileManager1"), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);
    QDBusConnection::sessionBus().interface()->registerService(QStringLiteral("org.freedesktop.FileManager1"), QDBusConnectionInterface::QueueService);
}

void FMDBusService::ShowFolders(const QStringList& uriList, const QString& startUpId)
{
    Q_EMIT showFolderRequest(uriList, startUpId);
}

void FMDBusService::ShowItems(const QStringList& uriList, const QString& startUpId)
{
    Q_EMIT showItemsRequest(uriList, startUpId);
}

void FMDBusService::ShowItemProperties(const QStringList& uriList, const QString& startUpId)
{
    Q_EMIT showItemPropertiesRequest(uriList, startUpId);
}
