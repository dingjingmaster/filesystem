#ifndef FILELAUNCHACTION_H
#define FILELAUNCHACTION_H

#include <QAction>
#include <gio/gio.h>


class FileLaunchAction : public QAction
{
    Q_OBJECT
public:
    explicit FileLaunchAction(const QString &uri, GAppInfo *app_info, bool forceWithArg = false, QObject *parent = nullptr);
    ~FileLaunchAction() override;

    GAppInfo *gAppInfo();
    const QString getUri();
    bool isDesktopFileAction();
    const QString getAppInfoName();
    const QString getAppInfoDisplayName();

protected:
    bool isValid();
    void execFile();
    void execFileInterm();

public Q_SLOTS:
    void lauchFileSync(bool forceWithArg = false, bool skipDialog = true);
    void lauchFileAsync(bool forceWithArg = false, bool skipDialog = true);

private:
    QIcon mIcon;
    QString mUri;
    QString mInfoName;
    GAppInfo *mAppInfo;
    bool mIsDesktopFile;
    QString mInfoDisplayName;
    bool mForceWithArg = false;
};

#endif // FILELAUNCHACTION_H
