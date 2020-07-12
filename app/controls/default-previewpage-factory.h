#ifndef DEFAULTPREVIEWPAGEFACTORY_H
#define DEFAULTPREVIEWPAGEFACTORY_H

#include <QObject>
#include <plugin-iface/preview-page-plugin-iface.h>


class DefaultPreviewPageFactory : public QObject, public PreviewPagePluginIface
{
    Q_OBJECT
public:
    static DefaultPreviewPageFactory *getInstance();

    PluginType pluginType() override {
        return PluginType::PreviewPagePlugin;
    }
    const QString name() override {
        return tr("Default Preview");
    }
    const QString description() override {
        return tr("This is the Default Preview of peony-qt");
    }
    const QIcon icon() override {
        return QIcon::fromTheme("ukui-preview-file", QIcon::fromTheme("preview-file"));
    }
    void setEnable(bool enable) override {
        mEnable = enable;
    }
    bool isEnable() override {
        return mEnable;
    }

    PreviewPageIface *createPreviewPage() override;

private:
    explicit DefaultPreviewPageFactory(QObject *parent = nullptr);
    ~DefaultPreviewPageFactory() override;

    bool mEnable = true;
};


#endif // DEFAULTPREVIEWPAGEFACTORY_H
