#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <QMutex>
#include <QObject>
#include <QObject>
#include <QSettings>
#include <qgsettings/QGSettings>

#define RESIDENT_IN_BACKEND         "resident"
#define SHOW_HIDDEN_PREFERENCE      "show-hidden"
#define SORT_FOLDER_FIRST           "folder-first"
#define SORT_CHINESE_FIRST          "chinese-first"
#define FORBID_THUMBNAIL_IN_VIEW    "do-not-thumbnail"
#define SIDEBAR_BG_OPACITY          "sidebar-bg-opacity"
#define DEFAULT_WINDOW_SIZE         "default-window-size"
#define DEFAULT_SIDEBAR_WIDTH       "default-sidebar-width"
#define ALLOW_FILE_OP_PARALLEL      "allow-file-op-parallel"
#define LAST_DESKTOP_SORT_ORDER     "last-desktop-sort-order"
#define DEFAULT_VIEW_ID             "directory-view/default-view-id"
#define DEFAULT_VIEW_ZOOM_LEVEL     "directory-view/default-view-zoom-level"

class GlobalSettings : public QObject
{
    Q_OBJECT
public:
    bool isExist (const QString &key);
    static GlobalSettings *getInstance ();
    const QVariant getValue (const QString &key);

Q_SIGNALS:
    void valueChanged (const QString &key);

public Q_SLOTS:
    void resetAll ();
    void reset (const QString &key);
    void forceSync (const QString &key = nullptr);
    void setValue (const QString &key, const QVariant &value);

private:
    explicit GlobalSettings (QObject *parent = nullptr);
    ~GlobalSettings ();

    QMutex mMutex;
    QSettings *mSettings;
    QMap<QString, QVariant> mCache;
    QGSettings *mGsettings = nullptr;
};

#endif // GLOBALSETTINGS_H
