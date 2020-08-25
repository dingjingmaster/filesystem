#include "directory-view-menu.h"

#include "directory-view-menu.h"
#include "plugin-iface/directory-view-plugin-iface2.h"
#include "directory-view-widget.h"

#include "main/fm-window.h"
#include "directory-view-container.h"

#include "menu-plugin-manager.h"
#include "file/file-info-job.h"
#include "file/file-info.h"

#include "view/view-factory/directory-view-factory-manager.h"
#include "view-factory-model.h"
#include "view-factory-sort-filter-model.h"

#include "common/clipbord-utils.h"
#include "file-operation-utils.h"
#include "file/file-operation-manager.h"

#include "file-utils.h"
#include "window/bookmark-manager.h"

#include "window/volume-manager.h"

#include "window/properties-window.h"

#include "file/file-launch-manager.h"
#include "file/file-launch-action.h"
#include "file/file-launch-dialog.h"

#include "file/file-operation-error-dialog.h"

#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>

#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

#include <QLocale>
#include <QStandardPaths>

#include <QDebug>

DirectoryViewMenu::DirectoryViewMenu(DirectoryViewWidget *directoryView, QWidget *parent) : QMenu(parent)
{
    mTopWindow = nullptr;
    mView = directoryView;

    mDirectory = directoryView->getDirectoryUri();
    mSelections = directoryView->getSelections();

    fillActions();
}

DirectoryViewMenu::DirectoryViewMenu(FMWindowIface *window, QWidget *parent) : QMenu(parent)
{
    mTopWindow = window;
    mView = window->getCurrentPage()->getView();
    //setParent(dynamic_cast<QWidget*>(mView));

    mDirectory = window->getCurrentUri();
    mSelections = window->getCurrentSelections();

    fillActions();
}

void DirectoryViewMenu::fillActions()
{
    if (mDirectory == "computer:///")
        mIsComputer = true;

    if (mDirectory == "trash:///")
        mIsTrash = true;

    if (mDirectory.startsWith("search://"))
        mIsSearch = true;

    if (mDirectory.startsWith("burn://"))
        mIsCd = true;

    //add open actions
    auto openActions = constructOpenOpActions();
    if (!openActions.isEmpty())
        addSeparator();

    //create template actions
    auto templateActions = constructCreateTemplateActions();
    if (!templateActions.isEmpty())
        addSeparator();

    //add view actions
    auto viewActions = constructViewOpActions();
    if (!viewActions.isEmpty())
        addSeparator();

    //add operation actions
    auto fileOpActions = constructFileOpActions();
    if (!fileOpActions.isEmpty())
        addSeparator();

    //add plugin actions
    auto pluginActions = constructMenuPluginActions();
    if (!pluginActions.isEmpty())
        addSeparator();

    //add propertries actions
    auto propertiesAction = constructFilePropertiesActions();
    if (!propertiesAction.isEmpty())
        addSeparator();

    //add actions in computer:///
    auto computerActions = constructComputerActions();
    if (!computerActions.isEmpty())
        addSeparator();

    //add actions in trash:///
    auto trashActions = constructTrashActions();
    if (!trashActions.isEmpty())
        addSeparator();

    //add actions in search:///
    auto searchActions = constructSearchActions();
}

const QList<QAction *> DirectoryViewMenu::constructOpenOpActions()
{
    QList<QAction *> l;
    if (mIsTrash)
        return l;

    bool isBackgroundMenu = mSelections.isEmpty();
    if (isBackgroundMenu) {
        l<<addAction(QIcon::fromTheme("window-new-symbolic"), tr("Open in &New Window"));
        connect(l.last(), &QAction::triggered, [=]() {
            auto windowIface = mTopWindow->create(mDirectory);
            auto newWindow = dynamic_cast<QWidget *>(windowIface);
            newWindow->setAttribute(Qt::WA_DeleteOnClose);
            //FIXME: show when prepared?
            newWindow->show();
        });
        l<<addAction(QIcon::fromTheme("tab-new-symbolic"), tr("Open in New &Tab"));
        connect(l.last(), &QAction::triggered, [=]() {
            if (!mTopWindow)
                return;
            QStringList uris;
            uris<<mDirectory;
            mTopWindow->addNewTabs(uris);
        });
    } else {
        if (mSelections.count() == 1) {
            auto info = FileInfo::fromUri(mSelections.first());
            auto displayName = info->displayName();
            if (displayName.isEmpty())
                displayName = FileUtils::getFileDisplayName(info->uri());
            //when name is too long, show elideText
            //qDebug() << "charWidth:" <<charWidth;
            if (displayName.length() > ELIDE_TEXT_LENGTH)
            {
                int  charWidth = fontMetrics().averageCharWidth();
                displayName = fontMetrics().elidedText(displayName, Qt::ElideRight, ELIDE_TEXT_LENGTH * charWidth);
            }
            if (info->isDir()) {
                //add to bookmark option
                l<<addAction(QIcon::fromTheme("bookmark-add-symbolic"), tr("Add to bookmark"));
                connect(l.last(), &QAction::triggered, [=]() {
                    //qDebug() <<"add to bookmark:" <<info->uri();
                    auto bookmark = BookMarkManager::getInstance();
                    if (bookmark->isLoaded()) {
                        bookmark->addBookMark(info->uri());
                    }
                });

                l<<addAction(QIcon::fromTheme("document-open-symbolic"), tr("&Open \"%1\"").arg(displayName));
                connect(l.last(), &QAction::triggered, [=]() {
                    if (!mTopWindow)
                        return;
                    mTopWindow->goToUri(mSelections.first(), true);
                });
                l<<addAction(QIcon::fromTheme("window-new-symbolic"), tr("Open \"%1\" in &New Window").arg(displayName));
                connect(l.last(), &QAction::triggered, [=]() {
                    auto windowIface = mTopWindow->create(mSelections.first());
                    auto newWindow = dynamic_cast<QWidget *>(windowIface);
                    newWindow->setAttribute(Qt::WA_DeleteOnClose);
                    //FIXME: show when prepared?
                    newWindow->show();
                });
                l<<addAction(QIcon::fromTheme("tab-new-symbolic"), tr("Open \"%1\" in New &Tab").arg(displayName));
                connect(l.last(), &QAction::triggered, [=]() {
                    if (!mTopWindow)
                        return;
                    mTopWindow->addNewTabs(mSelections);
                });
            } else if (!info->isVolume()) {
                l<<addAction(QIcon::fromTheme("document-open-symbolic"), tr("&Open \"%1\"").arg(displayName));
                connect(l.last(), &QAction::triggered, [=]() {
                    auto uri = mSelections.first();
                    FileLaunchManager::openAsync(uri, false, false);
                });
                auto openWithAction = addAction(tr("Open \"%1\" with...").arg(displayName));
                //FIXME: add sub menu for open with action.
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
                    //FIXME:
                    mTopWindow->goToUri(uri, true);
                });
            }
        } else {
            l<<addAction(QIcon::fromTheme("document-open-symbolic"), tr("&Open %1 selected files").arg(mSelections.count()));
            connect(l.last(), &QAction::triggered, [=]() {
                qDebug()<<"triggered";
                QStringList dirs;
                QMap<QString, QStringList> fileMap;
                /**step 1: Categorize files according to type.
                 * step 2: Open files in batches to avoid loss of asynchronous messages due to program startup.
                **/
                for (auto uri : mSelections) {
                    auto info = FileInfo::fromUri(uri);
                    if (info->isDir() || info->isVolume()) {
                        dirs<<uri;
                    } else {
                        QString mimeType = info->mimeType();
                        if (mimeType.isEmpty()) {
                            FileInfoJob job(info);
                            job.querySync();
                            mimeType = info->mimeType();
                        }

                        QStringList list;
                        if (fileMap.contains(mimeType)) {
                            list = fileMap[mimeType];
                            list << uri;
                            fileMap.insert(mimeType, list);
                        } else {
                            list << uri;
                            fileMap.insert(mimeType, list);
                        }
                    }
                }
                if (!dirs.isEmpty())
                    mTopWindow->addNewTabs(dirs);

                if(!fileMap.empty()) {
                    QMap<QString, QStringList>::iterator iter = fileMap.begin();
                    while (iter != fileMap.end())
                    {
                        FileLaunchManager::openAsync(iter.value());
                        iter++;
                    }
                }
            });
        }
    }

    return l;
}

const QList<QAction *> DirectoryViewMenu::constructCreateTemplateActions()
{
    QList<QAction *> l;
    if (mSelections.isEmpty()) {
        auto createAction = new QAction(tr("New..."), this);
        if (mIsCd) {
            createAction->setEnabled(false);
        }
        if(mDirectory.compare(QString::fromLocal8Bit("trash:///")) == 0)
        {
            createAction->setEnabled(false);
        }
        l<<createAction;
        QMenu *subMenu = new QMenu(this);
        createAction->setMenu(subMenu);
        addAction(createAction);

        //enumerate template dir
        QDir templateDir(g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES));
        auto templates = templateDir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot);
        if (!templates.isEmpty()) {
            for (auto t : templates) {
                QFileInfo qinfo(templateDir, t);
                GFile *gtk_file = g_file_new_for_path(qinfo.filePath().toUtf8().data());
                char *uri_str = g_file_get_uri(gtk_file);
                std::shared_ptr<FileInfo> info = FileInfo::fromUri(uri_str);

                QString mimeType = info->mimeType();
                if (mimeType.isEmpty()) {
                    FileInfoJob job(info);
                    job.querySync();
                    mimeType = info->mimeType();
                }

                QIcon tmpIcon;
                GList *app_infos = g_app_info_get_recommended_for_type(mimeType.toUtf8().constData());
                GList *l = app_infos;
                QList<FileLaunchAction *> actions;
                bool isOnlyUnref = false;
                while (l) {
                    auto app_info = static_cast<GAppInfo*>(l->data);
                    if (!isOnlyUnref) {
                        GThemedIcon *icon = G_THEMED_ICON(g_app_info_get_icon(app_info));
                        const char * const * icon_names = g_themed_icon_get_names(icon);
                        if (icon_names)
                            tmpIcon = QIcon::fromTheme(*icon_names);
                        if(!tmpIcon.isNull())
                            isOnlyUnref = true;
                    }
                    g_object_unref(app_infos);
                    l = l->next;
                }

                QAction *action = new QAction(tmpIcon, qinfo.baseName(), this);
                connect(action, &QAction::triggered, [=]() {
                    CreateTemplateOperation op(mDirectory, CreateTemplateOperation::Template, t);
                    FileOperationErrorDialogConflict dlg;
                    connect(&op, &FileOperation::errored, &dlg, &FileOperationErrorDialogConflict::handle);
                    op.run();
                    auto target = op.target();
                    mUrisToEdit<<target;
                });
                subMenu->addAction(action);
                g_free(uri_str);
                g_object_unref(gtk_file);
            }
            subMenu->addSeparator();
        }

        QList<QAction *> actions;
        auto createEmptyFileAction = new QAction(QIcon::fromTheme("document-new-symbolic"), tr("Empty &File"), this);
        actions<<createEmptyFileAction;
        connect(actions.last(), &QAction::triggered, [=]() {
            //FileOperationUtils::create(mDirectory);
            CreateTemplateOperation op(mDirectory);
            FileOperationErrorDialogConflict dlg;
            connect(&op, &FileOperation::errored, &dlg, &FileOperationErrorDialogConflict::handle);
            op.run();
            auto targetUri = op.target();
            qDebug()<<"target:"<<targetUri;
            mUrisToEdit<<targetUri;
        });
        auto createFolderActions = new QAction(QIcon::fromTheme("folder-new-symbolic"), tr("&Folder"), this);
        actions<<createFolderActions;
        connect(actions.last(), &QAction::triggered, [=]() {
            //FileOperationUtils::create(mDirectory, nullptr, CreateTemplateOperation::EmptyFolder);
            CreateTemplateOperation op(mDirectory, CreateTemplateOperation::EmptyFolder, tr("New Folder"));
            FileOperationErrorDialogConflict dlg;
            connect(&op, &FileOperation::errored, &dlg, &FileOperationErrorDialogConflict::handle);
            op.run();
            auto targetUri = op.target();
            qDebug()<<"target:"<<targetUri;
            mUrisToEdit<<targetUri;
        });
        subMenu->addActions(actions);
    }

    return l;
}

const QList<QAction *> DirectoryViewMenu::constructViewOpActions()
{
    QList<QAction *> l;

    if (mSelections.isEmpty()) {
        ViewFactorySortFilterModel2 model;
        model.setDirectoryUri(mDirectory);
        auto viewNames = model.supportViewIds();
        auto viewFactorysManager = DirectoryViewFactoryManager2::getInstance();

        if (!viewNames.isEmpty()) {
            //view type;
            auto viewTypeAction = addAction(tr("View Type..."));
            l<<viewTypeAction;
            QMenu *viewTypeSubMenu = new QMenu(this);
            for (auto viewId : viewNames) {
                auto viewType = viewTypeSubMenu->addAction(viewFactorysManager->getFactory(viewId)->viewIcon(), viewFactorysManager->getFactory(viewId)->viewName());
                viewType->setData(viewId);
                if (mView->viewId() == viewId) {
                    viewType->setCheckable(true);
                    viewType->setChecked(true);
                } else {
                    connect(viewType, &QAction::triggered, [=]() {
                        mTopWindow->slotBeginSwitchView(viewType->data().toString());
                    });
                }
            }
            viewTypeAction->setMenu(viewTypeSubMenu);
        }

        //sort type
        auto sortTypeAction = addAction(tr("Sort By..."));
        l<<sortTypeAction;
        QMenu *sortTypeMenu = new QMenu(this);

        QList<QAction *> tmp;
        tmp<<sortTypeMenu->addAction(tr("Name"));
        tmp<<sortTypeMenu->addAction(tr("File Size"));
        tmp<<sortTypeMenu->addAction(tr("File Type"));
        tmp<<sortTypeMenu->addAction(tr("Modified Date"));
        int sortType = mView->getSortType();
        if (sortType >= 0) {
            tmp.at(sortType)->setCheckable(true);
            tmp.at(sortType)->setChecked(true);
        }

        for (int i = 0; i < tmp.count(); i++) {
            connect(tmp.at(i), &QAction::triggered, [=]() {
                mTopWindow->slotSetCurrentSortColumn(i);
            });
        }

        sortTypeAction->setMenu(sortTypeMenu);

        //sort order
        auto sortOrderAction = addAction(tr("Sort Order..."));
        l<<sortOrderAction;
        QMenu *sortOrderMenu = new QMenu(this);
        tmp.clear();
        tmp<<sortOrderMenu->addAction(tr("Ascending Order"));
        tmp<<sortOrderMenu->addAction(tr("Descending Order"));
        int sortOrder = mView->getSortOrder();
        tmp.at(sortOrder)->setCheckable(true);
        tmp.at(sortOrder)->setChecked(true);

        for (int i = 0; i < tmp.count(); i++) {
            connect(tmp.at(i), &QAction::triggered, [=]() {
                mTopWindow->slotSetCurrentSortOrder(Qt::SortOrder(i));
            });
        }

        sortOrderAction->setMenu(sortOrderMenu);

        auto sortPreferencesAction = addAction(tr("Sort Preferences..."));
        l<<sortPreferencesAction;

        auto sortPreferencesMenu = new QMenu(this);
        auto folderFirst = sortPreferencesMenu->addAction(tr("Folder First"));
        folderFirst->setCheckable(true);
        folderFirst->setChecked(mTopWindow->getWindowSortFolderFirst());
        connect(folderFirst, &QAction::triggered, this, [=]() {
            mTopWindow->slotSetSortFolderFirst();
            folderFirst->setChecked(mTopWindow->getWindowSortFolderFirst());
        });

        if (QLocale::system().name().contains("zh")) {
            auto useDefaultNameSortOrder = sortPreferencesMenu->addAction(tr("Chinese First"));
            useDefaultNameSortOrder->setCheckable(true);
            useDefaultNameSortOrder->setChecked(mTopWindow->getWindowUseDefaultNameSortOrder());
            connect(useDefaultNameSortOrder, &QAction::triggered, this, [=]() {
                mTopWindow->slotSetUseDefaultNameSortOrder();
                bool checked = mTopWindow->getWindowUseDefaultNameSortOrder();
                useDefaultNameSortOrder->setChecked(checked);
            });
        }

        auto showHidden = sortPreferencesMenu->addAction(tr("Show Hidden"));
        showHidden->setCheckable(true);
        showHidden->setChecked(mTopWindow->getWindowShowHidden());
        connect(showHidden, &QAction::triggered, this, [=]() {
            mTopWindow->slotSetShowHidden();
            showHidden->setChecked(mTopWindow->getWindowShowHidden());
        });

        sortPreferencesAction->setMenu(sortPreferencesMenu);
    }

    return l;
}

const QList<QAction *> DirectoryViewMenu::constructFileOpActions()
{
    QList<QAction *> l;

    if (!mIsTrash && !mIsSearch && !mIsComputer) {
        QString homeUri = "file://" +  QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        if (!mSelections.isEmpty() && !mSelections.contains(homeUri)) {
            l<<addAction(QIcon::fromTheme("edit-copy-symbolic"), tr("&Copy"));
            connect(l.last(), &QAction::triggered, [=]() {
                ClipbordUtils::setClipboardFiles(mSelections, false);
            });
            l<<addAction(QIcon::fromTheme("edit-cut-symbolic"), tr("Cut"));
            connect(l.last(), &QAction::triggered, [=]() {
                ClipbordUtils::setClipboardFiles(mSelections, true);
            });
            l<<addAction(QIcon::fromTheme("edit-delete-symbolic"), tr("&Delete to trash"));
            connect(l.last(), &QAction::triggered, [=]() {
                FileOperationUtils::trash(mSelections, true);
            });
            //add delete forever option
            l<<addAction(QIcon::fromTheme("edit-clear-symbolic"), tr("Delete forever"));
            connect(l.last(), &QAction::triggered, [=]() {
                FileOperationUtils::executeRemoveActionWithDialog(mSelections);
            });
            if (mSelections.count() == 1) {
                l<<addAction(QIcon::fromTheme("document-edit-symbolic"), tr("Rename"));
                connect(l.last(), &QAction::triggered, [=]() {
                    mView->editUri(mSelections.first());
                });
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
                mTopWindow->slotRefresh();
            });
        }
    }

    //select all and reverse select
    if (mSelections.isEmpty())
    {
        l<<addAction(tr("Select &All"));
        connect(l.last(), &QAction::triggered, [=]() {
            //qDebug() << "select all";
            mView->invertSelections();
        });
    }
    else
    {
        l<<addAction(tr("Reverse Select"));
        connect(l.last(), &QAction::triggered, [=]() {
            //qDebug() << "Reverse select";
            mView->invertSelections();
        });
    }

    return l;
}

const QList<QAction *> DirectoryViewMenu::constructFilePropertiesActions()
{
    QList<QAction *> l;

    if (!mIsSearch) {
        l<<addAction(QIcon::fromTheme("preview-file"), tr("Properties"));
        connect(l.last(), &QAction::triggered, [=]() {
            //FIXME:
            if (mSelections.isEmpty()) {
                QStringList uris;
                uris<<mDirectory;
                PropertiesWindow *p = new PropertiesWindow(uris);
                p->setAttribute(Qt::WA_DeleteOnClose);
                p->show();
            } else {
                PropertiesWindow *p = new PropertiesWindow(mSelections);
                p->setAttribute(Qt::WA_DeleteOnClose);
                p->show();
            }
        });
    } else if (mSelections.count() == 1) {
        l<<addAction(QIcon::fromTheme("preview-file"), tr("Properties"));
        connect(l.last(), &QAction::triggered, [=]() {
            PropertiesWindow *p = new PropertiesWindow(mSelections);
            p->setAttribute(Qt::WA_DeleteOnClose);
            p->show();
        });
    }

    return l;
}

const QList<QAction *> DirectoryViewMenu::constructComputerActions()
{
    QList<QAction *> l;

    if (mIsComputer) {
        /*
        //FIXME: get the volume state and add action dynamicly.
        if (mSelections.count() == 1) {
            l<<addAction(tr("&Umount"));
            connect(l.last(), &QAction::triggered, [=](){
                VolumeManager::unmount(mSelections.first());
            });
            l<<addAction(tr("&Format"));
            connect(l.last(), &QAction::triggered, [=](){
                //FIXME:
                //add format function?
                //maybe put it in plugin...
            });
        }
        */
    }

    return l;
}

const QList<QAction *> DirectoryViewMenu::constructTrashActions()
{
    QList<QAction *> l;

    if (mIsTrash) {
        bool isTrashEmpty = mTopWindow->getCurrentAllFileUris().isEmpty();

        if (mSelections.isEmpty()) {
            l<<addAction(QIcon::fromTheme("window-close-symbolic"), tr("&Clean the Trash"));
            l.last()->setEnabled(!isTrashEmpty);
            connect(l.last(), &QAction::triggered, [=]() {
                auto result = QMessageBox::question(nullptr, tr("Delete Permanently"), tr("Are you sure that you want to delete these files? "
                                                                                          "Once you start a deletion, the files deleting will never be "
                                                                                          "restored again."));
                if (result == QMessageBox::Yes) {
                    auto uris = mTopWindow->getCurrentAllFileUris();
                    FileOperationUtils::remove(uris);
                }
            });
        } else {
            l<<addAction(tr("&Restore"));
            connect(l.last(), &QAction::triggered, [=]() {
                if (mSelections.count() == 1) {
                    FileOperationUtils::restore(mSelections.first());
                } else {
                    FileOperationUtils::restore(mSelections);
                }
            });
            l<<addAction(QIcon::fromTheme("window-close-symbolic"), tr("&Delete"));
            connect(l.last(), &QAction::triggered, [=]() {
                auto result = QMessageBox::question(nullptr, tr("Delete Permanently"), tr("Are you sure that you want to delete these files? "
                                                                                          "Once you start a deletion, the files deleting will never be "
                                                                                          "restored again."));
                if (result == QMessageBox::Yes) {
                    FileOperationUtils::remove(mSelections);
                }
            });
        }
    }

    return l;
}

const QList<QAction *> DirectoryViewMenu::constructSearchActions()
{
    QList<QAction *> l;
    if (mIsSearch) {
        if (mSelections.isEmpty())
            return l;

        l<<addAction(QIcon::fromTheme("new-window-symbolc"), tr("Open Parent Folder in New Window"));
        connect(l.last(), &QAction::triggered, [=]() {
            for (auto uri : mSelections) {
                auto parentUri = FileUtils::getParentUri(uri);
                if (!parentUri.isNull()) {
                    auto *windowIface = mTopWindow->create(parentUri);
                    auto newWindow = dynamic_cast<QWidget *>(windowIface);
                    auto selection = mSelections;
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
                    QTimer::singleShot(1000, newWindow, [=]() {
                        if (newWindow)
                            windowIface->slotSetCurrentSelectionUris(selection);
                    });
#else
                    QTimer::singleShot(1000, [=]() {
                        if (newWindow)
                            windowIface->slotSetCurrentSelectionUris(selection);
                    });
#endif
                    newWindow->show();
                }
            }
        });
    }
    return l;
}

const QList<QAction *> DirectoryViewMenu::constructMenuPluginActions()
{
    QList<QAction *> l;
    auto pluginIds = MenuPluginManager::getInstance()->getPluginIds();
    //sort plugiins by name, so the menu option orders is relatively fixed
    qSort(pluginIds.begin(), pluginIds.end());

    for (auto id : pluginIds) {
        auto plugin = MenuPluginManager::getInstance()->getPlugin(id);
        auto actions = plugin->menuActions(MenuPluginInterface::DirectoryView, mDirectory, mSelections);
        l<<actions;
        for (auto action : actions) {
            action->setParent(this);
            addAction(action);
        }
    }
    return l;
}
