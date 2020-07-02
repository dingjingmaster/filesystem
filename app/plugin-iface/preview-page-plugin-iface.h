#ifndef PREVIEWPAGEPLUGINIFACE_H
#define PREVIEWPAGEPLUGINIFACE_H

#include "plugin-iface.h"

class QString;
class PreviewPageIface;

#define PreviewPagePluginIface_iid "org.graceful.fm.plugin-iface.PreviewPagePluginIface"


class PreviewPagePluginIface : public PluginIface
{
public:
    virtual ~PreviewPagePluginIface() {}

    virtual PreviewPageIface *createPreviewPage() = 0;
};

class PreviewPageIface
{
public:
    enum PreviewType
    {
        PDF,
        Text,
        Pictrue,
        Attribute,
        OfficeDoc,
        Other
    };

    virtual ~PreviewPageIface() {}

    virtual void cancel() = 0;
    virtual void startPreview() = 0;
    virtual void closePreviewPage() = 0;
    virtual void prepare(const QString &uri) = 0;
    virtual void prepare(const QString &uri, PreviewType type) = 0;
};

Q_DECLARE_INTERFACE(PreviewPagePluginIface, PreviewPagePluginIface_iid)


#endif // PREVIEWPAGEPLUGINIFACE_H
