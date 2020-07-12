#ifndef PREVIEWPAGEFACTORYMANAGER_H
#define PREVIEWPAGEFACTORYMANAGER_H

#include <QObject>
#include <plugin-iface/preview-page-plugin-iface.h>


class PreviewPageFactoryManager : public QObject
{
    Q_OBJECT
public:
    static PreviewPageFactoryManager *getInstance();

    const QStringList getPluginNames();
    const QString getLastPreviewPageId();
    PreviewPagePluginIface *getPlugin(const QString &name);
    bool registerFactory(const QString &name, PreviewPagePluginIface* plugin);

private:
    explicit PreviewPageFactoryManager(QObject *parent = nullptr);
    ~PreviewPageFactoryManager();

private:
    QString mLastPreviewPageId = nullptr;
    QMap<QString, PreviewPagePluginIface*> *mMap = nullptr;
};

#endif // PREVIEWPAGEFACTORYMANAGER_H
