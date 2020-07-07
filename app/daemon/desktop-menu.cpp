#include "desktop-icon-view.h"

#include "desktop-menu.h"
#include "file/file-launch-action.h"
#include "desktop-menu-plugin-manager.h"

#include <QUrl>
#include <QDir>
#include <memory>
#include <QAction>
#include <QProcess>
#include <QMessageBox>
#include <file-utils.h>
#include <QStandardPaths>
#include <file/file-info.h>
#include <clipbord-utils.h>
#include <global-settings.h>
#include <QFileIconProvider>
#include <file-operation-utils.h>
#include <file/file-enumerator.h>
#include <file/file-launch-dialog.h>
#include <file/file-launch-manager.h>
#include <file/file-launch-manager.h>

DesktopMenu::DesktopMenu(DirectoryViewIface *view, QWidget *parent) : QMenu(parent)
{
    setProperty("useIconHighlightEffect", true);
    setProperty("iconHighlightEffectMode", 1);
    setProperty("fillIconSymbolicColor", true);

    mView = view;
    mDirectory = view->getDirectoryUri();
    mSelections = view->getSelections();

    fillActions();
}

const QStringList DesktopMenu::urisToEdit()
{
    return mUrisToEdit;
}

void DesktopMenu::fillActions()
{
    auto openActions = constructOpenOpActions();
    if (!openActions.isEmpty()) {
        addSeparator();
    }

    auto createTemplateActions = constructCreateTemplateActions();
    if (!createTemplateActions.isEmpty()) {
        addSeparator();
    }

    auto viewActions = constructViewOpActions();
    if (!viewActions.isEmpty()) {
        addSeparator();
    }

    auto fileOpActions = constructFileOpActions();
    if (!fileOpActions.isEmpty()) {
        addSeparator();
    }

    auto pluginActions = constructMenuPluginActions();
    if (!pluginActions.isEmpty()) {
        addSeparator();
    }

    bool isBackgroundMenu = mSelections.isEmpty();
    if(!isBackgroundMenu) {
        auto propertiesAction = constructFilePropertiesActions();
    }
}

const QList<QAction *> DesktopMenu::constructOpenOpActions()
{
    QList<QAction *> l;

    bool isBackgroundMenu = mSelections.isEmpty();
    if (isBackgroundMenu) {
        l<<addAction(QIcon::fromTheme("window-new-symbolic"), tr("&Open in new Window"));
        connect(l.last(), &QAction::triggered, [=]() {
            this->openWindow(mDirectory);
        });

        l<<addAction(tr("Select &All"));
        connect(l.last(), &QAction::triggered, [=]() {
            mView->invertSelections();
        });
    } else {
        if (mSelections.count() == 1) {
            auto info = FileInfo::fromUri(mSelections.first());
            auto displayName = info->displayName();
            if (displayName.isEmpty()) {
                displayName = FileUtils::getFileDisplayName(info->uri());
            }

            if (displayName.length() > ELIDE_TEXT_LENGTH) {
                int  charWidth = fontMetrics().averageCharWidth();
                displayName = fontMetrics().elidedText(displayName, Qt::ElideRight, ELIDE_TEXT_LENGTH * charWidth);
            }
            if (info->isDir()) {
                l<<addAction(QIcon::fromTheme("document-open-symbolic"), tr("&Open \"%1\"").arg(displayName));
                connect(l.last(), &QAction::triggered, [=]() {
                    this->openWindow(mSelections);
                });

                auto openWithAction = addAction(tr("Open \"%1\" &with...").arg(displayName));
                QMenu *openWithMenu = new QMenu(this);
                auto recommendActions = FileLaunchManager::getRecommendActions(mSelections.first());
                for (auto action : recommendActions) {
                    action->setParent(openWithMenu);
                    openWithMenu->addAction(static_cast<QAction*>(action));
                }
                auto fallbackActions = FileLaunchManager::getFallbackActions(mSelections.first());
                for (auto action : fallbackActions) {
                    action->setParent(openWithMenu);
                    openWithMenu->addAction(static_cast<QAction*>(action));
                }
                openWithMenu->addSeparator();
                openWithMenu->addAction(tr("&More applications..."), [=]() {
                    FileLauchDialog d(mSelections.first());
                    d.exec();
                });
                openWithAction->setMenu(openWithMenu);
            } else if (!info->isVolume()) {
                l<<addAction(QIcon::fromTheme("document-open-symbolic"), tr("&Open \"%1\"").arg(displayName));
                connect(l.last(), &QAction::triggered, [=]() {
                    auto uri = mSelections.first();
                    FileLaunchManager::openAsync(uri);
                });
                auto openWithAction = addAction(tr("Open \"%1\" with...").arg(displayName));
                QMenu *openWithMenu = new QMenu(this);
                auto recommendActions = FileLaunchManager::getRecommendActions(mSelections.first());
                for (auto action : recommendActions) {
                    action->setParent(openWithMenu);
                    openWithMenu->addAction(static_cast<QAction*>(action));
                }
                auto fallbackActions = FileLaunchManager::getFallbackActions(mSelections.first());
                for (auto action : fallbackActions) {
                    action->setParent(openWithMenu);
                    openWithMenu->addAction(static_cast<QAction*>(action));
                }
                openWithMenu->addSeparator();
                openWithMenu->addAction(tr("&More applications..."), [=]() {
                    FileLauchDialog d(mSelections.first());
                    d.exec();
                });
                openWithAction->setMenu(openWithMenu);
            } else {
                l<<addAction(tr("&Open"));
                connect(l.last(), &QAction::triggered, [=]() {
                    auto uri = mSelections.first();
                });
            }
        } else {
            l<<addAction(QIcon::fromTheme("document-open-symbolic"), tr("&Open %1 selected files").arg(mSelections.count()));
            connect(l.last(), &QAction::triggered, [=]() {
                QStringList dirs;
                QStringList files;
                for (auto uri : mSelections) {
                    auto info = FileInfo::fromUri(uri);
                    if (info->isDir() || info->isVolume()) {
                        dirs<<uri;
                    } else {
                        files<<uri;
                    }
                }
                if (!dirs.isEmpty())
                    this->openWindow(dirs);
                if (!files.isEmpty()) {
                    for (auto uri : files) {
                        FileLaunchManager::openAsync(uri);
                    }
                }
            });
        }

        l << addAction(tr("Reverse Select"));
        connect(l.last(), &QAction::triggered, [=]() {
            mView->invertSelections();
        });
    }

    return l;
}

const QList<QAction *> DesktopMenu::constructViewOpActions()
{
    QList<QAction *> l;

    if (mSelections.isEmpty()) {
        auto viewTypeAction = addAction(tr("View Type..."));
        l << viewTypeAction;
        QMenu *viewTypeSubMenu = new QMenu(this);
        auto desktopView = dynamic_cast<DesktopIconView*>(mView);
        auto zoomLevel = desktopView->zoomLevel();

        auto smallAction = viewTypeSubMenu->addAction(tr("&Small"), [=]() {
            desktopView->setDefaultZoomLevel(DesktopIconView::Small);
        });
        auto normalAction = viewTypeSubMenu->addAction(tr("&Normal"), [=]() {
            desktopView->setDefaultZoomLevel(DesktopIconView::Normal);
        });
        auto largeAction = viewTypeSubMenu->addAction(tr("&Large"), [=]() {
            desktopView->setDefaultZoomLevel(DesktopIconView::Large);
        });
        auto hugeAction = viewTypeSubMenu->addAction(tr("&Huge"), [=]() {
            desktopView->setDefaultZoomLevel(DesktopIconView::Huge);
        });

        switch (zoomLevel) {
        case DesktopIconView::Small:
            smallAction->setCheckable(true);
            smallAction->setChecked(true);
            break;
        case DesktopIconView::Normal:
            normalAction->setCheckable(true);
            normalAction->setChecked(true);
            break;
        case DesktopIconView::Large:
            largeAction->setCheckable(true);
            largeAction->setChecked(true);
            break;
        case DesktopIconView::Huge:
            hugeAction->setCheckable(true);
            hugeAction->setChecked(true);
            break;
        default:
            break;
        }

        viewTypeAction->setMenu(viewTypeSubMenu);

        auto sortTypeAction = addAction(tr("Sort By..."));
        l<<sortTypeAction;
        QMenu *sortTypeMenu = new QMenu(this);

        QList<QAction *> tmp;
        tmp << sortTypeMenu->addAction(tr("Name"));
        tmp << sortTypeMenu->addAction(tr("File Type"));
        tmp << sortTypeMenu->addAction(tr("File Size"));
        tmp << sortTypeMenu->addAction(tr("Modified Date"));

        for (int i = 0; i < tmp.count(); i++) {
            connect(tmp.at(i), &QAction::triggered, [=]() {
                mView->setSortType(i);
                GlobalSettings::getInstance()->setValue(LAST_DESKTOP_SORT_ORDER, i);
            });
        }

        sortTypeAction->setMenu(sortTypeMenu);
    }

    return l;
}

const QList<QAction *> DesktopMenu::constructFileOpActions()
{
    QList<QAction *> l;

    if (!mSelections.isEmpty()) {
        QString homeUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        if (mSelections.count() == 1 && mSelections.first() == "trash:///") {
            FileEnumerator e;
            e.setEnumerateDirectory("trash:///");
            e.enumerateSync();
            auto trashChildren = e.getChildrenUris();
            l<<addAction(QIcon::fromTheme("view-refresh-symbolic"), tr("&Restore all"), [=]() {
                FileOperationUtils::restore(trashChildren);
            });
            l.last()->setEnabled(!trashChildren.isEmpty());
            l<<addAction(QIcon::fromTheme("edit-clear-symbolic"), tr("&Clean the trash"), [=]() {
                auto result = QMessageBox::question(nullptr, tr("Delete Permanently"), tr("Are you sure that you want to delete these files? "
                                                    "Once you start a deletion, the files deleting will never be "
                                                    "restored again."));
                if (result == QMessageBox::Yes) {
                    FileEnumerator e;
                    FileOperationUtils::remove(trashChildren);
                }
            });
            l.last()->setEnabled(!trashChildren.isEmpty());
        } else if (mSelections.count() == 1 && mSelections.first() == "computer:///") {

        } else if (! mSelections.contains(homeUri)) {
            l<<addAction(QIcon::fromTheme("edit-copy-symbolic"), tr("&Copy"));
            connect(l.last(), &QAction::triggered, [=]() {
                ClipbordUtils::setClipboardFiles(mSelections, false);
            });
            l<<addAction(QIcon::fromTheme("edit-cut-symbolic"), tr("Cu&t"));
            connect(l.last(), &QAction::triggered, [=]() {
                ClipbordUtils::setClipboardFiles(mSelections, true);
            });

            if (!mSelections.contains("trash:///")) {
                l<<addAction(QIcon::fromTheme("edit-clear-symbolic"), tr("&Delete to trash"));
                connect(l.last(), &QAction::triggered, [=]() {
                    FileOperationUtils::trash(mSelections, true);
                });
                //add delete forever option
                l<<addAction(QIcon::fromTheme("edit-delete-symbolic"), tr("Delete forever"));
                connect(l.last(), &QAction::triggered, [=]() {
                    FileOperationUtils::executeRemoveActionWithDialog(mSelections);
                });
            }

            if (mSelections.count() == 1) {
                l<<addAction(QIcon::fromTheme("document-edit-symbolic"), tr("&Rename"));
                connect(l.last(), &QAction::triggered, [=]() {
                    mView->editUri(mSelections.first());
                });
            }
        }
    } else {
        auto pasteAction = addAction(QIcon::fromTheme("edit-paste-symbolic"), tr("&Paste"));
        l<<pasteAction;
        pasteAction->setEnabled(ClipbordUtils::isClipboardHasFiles());
        connect(l.last(), &QAction::triggered, [=]() {
            ClipbordUtils::pasteClipboardFiles(mDirectory);
        });
        l<<addAction(QIcon::fromTheme("view-refresh-symbolic"), tr("&Refresh"));
        connect(l.last(), &QAction::triggered, [=]() {
            auto desktopView = dynamic_cast<DesktopIconView*>(mView);
            desktopView->refresh();
        });
    }

    return l;
}

const QList<QAction *> DesktopMenu::constructMenuPluginActions()
{
    QList<QAction *> l;
    auto mgr = DesktopMenuPluginManager::getInstance();
    if (mgr->isLoaded()) {
        auto pluginIds = mgr->getPluginIds();
        qSort(pluginIds.begin(), pluginIds.end());

        for (auto id : pluginIds) {
            auto plugin = mgr->getPlugin(id);
            auto actions = plugin->menuActions(MenuPluginIface::DesktopWindow, mDirectory, mSelections);
            for (auto action : actions) {
                action->setParent(this);
            }

            l << actions;
        }
    }
    addActions(l);
    return l;
}

const QList<QAction *> DesktopMenu::constructCreateTemplateActions()
{
    QList<QAction *> l;
    if (mSelections.isEmpty()) {
        auto createAction = new QAction(tr("&New..."), this);
        l<<createAction;
        QMenu *subMenu = new QMenu(this);
        createAction->setMenu(subMenu);
        addAction(createAction);

        //enumerate template dir
        QDir templateDir(g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES));
        auto templates = templateDir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot);
        if (!templates.isEmpty()) {
            QFileIconProvider p;
            for (auto t : templates) {
                QFileInfo info(t);
                QAction *action = new QAction(p.icon(info), info.baseName(), this);
                connect(action, &QAction::triggered, [=]() {
                    FileCreateTemplOperation op(mDirectory, FileCreateTemplOperation::Template, t);
                    op.run();
                    auto target = op.target();
                    mUrisToEdit<<target;
                });
                subMenu->addAction(action);
            }
            subMenu->addSeparator();
        }

        QList<QAction *> actions;
        auto createEmptyFileAction = new QAction(QIcon::fromTheme("document-new-symbolic"), tr("Empty &File"), this);
        actions<<createEmptyFileAction;
        connect(actions.last(), &QAction::triggered, [=]() {
            //FileOperationUtils::create(mDirectory);
            FileCreateTemplOperation op(mDirectory);
            op.run();
            auto targetUri = op.target();
            mUrisToEdit<<targetUri;
        });
        auto createFolderActions = new QAction(QIcon::fromTheme("folder-new-symbolic"), tr("&Folder"), this);
        actions<<createFolderActions;
        connect(actions.last(), &QAction::triggered, [=]() {
            //FileOperationUtils::create(mDirectory, nullptr, CreateTemplateOperation::EmptyFolder);
            FileCreateTemplOperation op(mDirectory, FileCreateTemplOperation::EmptyFolder, tr("New Folder"));
            op.run();
            auto targetUri = op.target();
            mUrisToEdit<<targetUri;
        });
        subMenu->addActions(actions);
    }

    return l;
}

const QList<QAction *> DesktopMenu::constructFilePropertiesActions()
{
    QList<QAction *> l;

    l<<addAction(QIcon::fromTheme("preview-file"), tr("&Properties"));
    connect(l.last(), &QAction::triggered, [=]() {
        if (mSelections.isEmpty()) {
            this->showProperties(mDirectory);
        } else {
            this->showProperties(mSelections);
        }
    });

    return l;
}

void DesktopMenu::gotoAboutComputer()
{

}

void DesktopMenu::openWindow(const QString &uri)
{
    QUrl url = uri;
    QProcess p;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    p.setProgram("filesystem");
    p.setArguments(QStringList()<<"--show-folders"<<url.toEncoded());
    p.startDetached();
#else
    p.startDetached("filesystem", QStringList()<<"--show-folders"<<uri);
#endif
}

void DesktopMenu::showProperties(const QString &uri)
{
    if (uri == "computer:///" || uri == "//") {
        gotoAboutComputer();
        return;
    }

    QUrl url = uri;
    QProcess p;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    p.setProgram("filesystem");
    p.setArguments(QStringList()<<"--show-properties"<<url.toEncoded());
    p.startDetached();
#else
    p.startDetached("peony", QStringList()<<"--show-properties"<<url.toEncoded());
#endif
}

void DesktopMenu::openWindow(const QStringList &uris)
{
    QStringList args;
    for (auto arg : uris) {
        QUrl url = arg;
        args<<QString(url.toEncoded());
    }
    QProcess p;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    p.setProgram("filesystem");
    p.setArguments(QStringList()<<"--show-folders"<<args);
    p.startDetached();
#else
    p.startDetached("filesystem", QStringList()<<"--show-folders"<<args);
#endif
}

void DesktopMenu::showProperties(const QStringList &uris)
{
    QStringList args;
    for (auto arg : uris) {
        QUrl url = arg;
        args<<url.toEncoded();
    }

    if (uris.contains("computer:///")) {
        gotoAboutComputer();
        return;
    }

    QProcess p;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    p.setProgram("filesystem");
    p.setArguments(QStringList()<<"--show-properties"<<args);
    p.startDetached();
#else
    p.startDetached("filesystem", QStringList()<<"--show-properties"<<args);
#endif
}
