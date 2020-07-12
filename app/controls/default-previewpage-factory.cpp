#include "default-previewpage-factory.h"
#include "default-previewpage.h"

static DefaultPreviewPageFactory *gInstance = nullptr;

DefaultPreviewPageFactory *DefaultPreviewPageFactory::getInstance()
{
    if (!gInstance) {
        gInstance = new DefaultPreviewPageFactory;
    }
    return gInstance;
}

DefaultPreviewPageFactory::DefaultPreviewPageFactory(QObject *parent) : QObject(parent)
{

}

DefaultPreviewPageFactory::~DefaultPreviewPageFactory()
{

}

PreviewPageIface *DefaultPreviewPageFactory::createPreviewPage()
{
    return new DefaultPreviewPage;
}
