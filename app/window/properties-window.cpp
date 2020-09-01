#include "properties-window.h"

//#include "properties-window-tab-page-plugin-iface.h"

//#include "basic-properties-page-factory.h"
//#include "permissions-properties-page-factory.h"
//#include "computer-properties-page-factory.h"
//#include "recent-and-trash-properties-page-factory.h"

#include <QToolBar>
#include <QProcess>
#include <QPushButton>
#include <QDialogButtonBox>
#include <plugin-iface/properties-window-tab-page-plugin-iface.h>

static PropertiesWindowPluginManager *global_instance = nullptr;

PropertiesWindowPrivate::PropertiesWindowPrivate(const QStringList &uris, QWidget *parent) : QTabWidget(parent)
{
    setTabsClosable(false);
    setMovable(false);

    auto manager = PropertiesWindowPluginManager::getInstance();
    auto names = manager->getFactoryNames();
    for (auto name : names) {
        auto factory = manager->getFactory(name);
        if (factory->supportUris(uris)) {
            auto tabPage = factory->createTabPage(uris);
            tabPage->setParent(this);
            addTab(tabPage, factory->name());
        }
    }
}

void PropertiesWindow::show()
{
    if (mUris.contains("computer:///")) {
        close();
    } else {
        return QWidget::show();
    }
}

void PropertiesWindow::gotoAboutComputer()
{

}

void PropertiesWindowPluginManager::release()
{
    deleteLater();
}

const QStringList PropertiesWindowPluginManager::getFactoryNames()
{
    QStringList l;
    for (auto factoryId : mSortedFactoryMap) {
        l << factoryId;
    }
    return l;
}

PropertiesWindowPluginManager *PropertiesWindowPluginManager::getInstance()
{
    if (!global_instance)
        global_instance = new PropertiesWindowPluginManager;
    return global_instance;
}

bool PropertiesWindowPluginManager::registerFactory(PropertiesWindowTabPagePluginIface *factory)
{
    mMutex.lock();
    auto id = factory->name();
    if (mFactoryHash.value(id)) {
        mMutex.unlock();
        return false;
    }

    mFactoryHash.insert(id, factory);
    mSortedFactoryMap.insert(-factory->tabOrder(), id);
    mMutex.unlock();
    return true;
}

PropertiesWindowTabPagePluginIface *PropertiesWindowPluginManager::getFactory(const QString &id)
{
    return mFactoryHash.value(id);
}

PropertiesWindowPluginManager::~PropertiesWindowPluginManager()
{
    for (auto factory : mFactoryHash) {
        factory->closeFactory();
    }
    mFactoryHash.clear();
}

PropertiesWindowPluginManager::PropertiesWindowPluginManager(QObject *parent)
{
//    registerFactory(BasicPropertiesPageFactory::getInstance());
//    registerFactory(PermissionsPropertiesPageFactory::getInstance());
//    registerFactory(ComputerPropertiesPageFactory::getInstance());
//    registerFactory(RecentAndTrashPropertiesPageFactory::getInstance());
}

PropertiesWindow::PropertiesWindow(const QStringList &uris, QWidget *parent) : QMainWindow(parent)
{
    mUris = uris;
    if (uris.contains("computer:///")) {
        gotoAboutComputer();
    } else {
        setWindowIcon(QIcon::fromTheme("system-file-manager"));
        setWindowTitle(tr("Properties"));

        setAttribute(Qt::WA_DeleteOnClose);
        setContentsMargins(0, 15, 0, 0);
        setMinimumSize(360, 480);
        auto w = new PropertiesWindowPrivate(uris, this);
        setCentralWidget(w);

        QToolBar *buttonToolBar = new QToolBar(this);
        addToolBar(Qt::BottomToolBarArea, buttonToolBar);

        QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Close, buttonToolBar);
        box->setContentsMargins(0, 5, 5, 5);
        box->button(QDialogButtonBox::Close)->setText(tr("Close"));
        buttonToolBar->addWidget(box);

        connect(box, &QDialogButtonBox::rejected, [=]() {
            this->close();
        });
    }
}
