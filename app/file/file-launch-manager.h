#ifndef FILELAUNCHMANAGER_H
#define FILELAUNCHMANAGER_H
#include <QObject>

class FileLaunchAction;

class FileLaunchManager : public QObject
{
    Q_OBJECT
public:
    static FileLaunchAction *getDefaultAction(const QString &uri);
    static const QList<FileLaunchAction*> getAllActions(const QString &uri);
    static const QList<FileLaunchAction*> getFallbackActions(const QString &uri);
    static const QList<FileLaunchAction*> getRecommendActions(const QString &uri);
    static const QList<FileLaunchAction*> getAllActionsForType(const QString &uri);
    static void setDefaultLauchAction(const QString &uri, FileLaunchAction *action);
    static void openSync(const QString &uri, bool forceWithArg = false, bool skipDialog = true);
    static void openAsync(const QString &uri, bool forceWithArg = false, bool skipDialog = true);

private:
    explicit FileLaunchManager(QObject *parent = nullptr);
};


#endif // FILELAUNCHMANAGER_H
