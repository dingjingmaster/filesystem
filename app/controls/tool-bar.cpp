#include "tool-bar.h"

#include "tool-bar.h"
#include "search-bar.h"
#include "main/fm-window.h"
#include "view-factory/directory-view-factory-manager.h"
#include "plugin-iface/directory-view-plugin-iface.h"
#include "plugin-iface/directory-view-plugin-iface2.h"
#include "directory-view-widget.h"
#include "common/clipbord-utils.h"
#include "file-operation-utils.h"

#include "model/view-factory-model.h"
#include "view-factory-sort-filter-model.h"
#include "directory-view-container.h"
#include "global-settings.h"

#include <QAction>
#include <QComboBox>
#include <QPushButton>

#include <QStandardPaths>

#include <QMessageBox>

#include <QMenu>

#include <QUrl>
#include <QDesktopServices>

#include <QApplication>

#include <QDebug>

ToolBar::ToolBar(FMWindow *window, QWidget *parent) : QToolBar(parent)
{
    setContentsMargins(0, 0, 0, 0);
    setFixedHeight(50);

    mTopWindow = window;
    init();
}

void ToolBar::init()
{
    //layout
    QAction *newWindowAction = addAction(QIcon::fromTheme("window-new-symbolic"),
                                         tr("Open in &New window"));
    QAction *newTabActon = addAction(QIcon::fromTheme("tab-new-symbolic"),
                                     tr("Open in new &Tab"));

    addSeparator();

    //view switch
    //FIXME: how about support uri?
    auto viewManager = DirectoryViewFactoryManager2::getInstance();

    auto defaultViewId = viewManager->getDefaultViewId();

    auto model = new ViewFactorySortFilterModel2(this);
    mViewFactoryModel = model;
    model->setDirectoryUri("file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation));

    /*
    QComboBox *viewCombox = new QComboBox(this);
    m_view_option_box = viewCombox;
    viewCombox->setToolTip(tr("Change Directory View"));

    viewCombox->setModel(model);

    addWidget(viewCombox);
    connect(viewCombox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){
        auto viewId = viewCombox->itemData(index, Qt::ToolTipRole).toString();
        mTopWindow->beginSwitchView(viewId);
    });

    addSeparator();
    */

    mViewAction = new QAction(mViewFactoryModel->iconFromViewId(mTopWindow->getCurrentPageViewType()),
                                mTopWindow->getCurrentPageViewType(), this);
    mViewMenu = new QMenu(this);
    mViewAction->setMenu(mViewMenu);
    connect(mViewAction, &QAction::triggered, [=]() {
        auto point = this->widgetForAction(mViewAction)->geometry().bottomLeft();
        auto global_point = mapToGlobal(point);
        mViewMenu->exec(global_point);
    });
    connect(mViewMenu, &QMenu::aboutToShow, [=]() {
        for (auto id : mViewFactoryModel->supportViewIds()) {
            auto action = mViewMenu->addAction(mViewFactoryModel->iconFromViewId(id), id, [=]() {
                mTopWindow->getCurrentPage()->switchViewType(id);
            });
            if (id == mTopWindow->getCurrentPageViewType()) {
                action->setCheckable(true);
                action->setChecked(true);
            }
        }
    });
    connect(mViewMenu, &QMenu::aboutToHide, [=]() {
        for (auto action : mViewMenu->actions()) {
            action->deleteLater();
        }
    });

    addAction(mViewAction);
    addSeparator();

    //sort type
    /*!
      \todo
      make column extensionable.
      */
    mSortAction = new QAction(QIcon::fromTheme("view-sort-ascending-symbolic"), tr("Sort Type"), this);
    QMenu *sortMenu = new QMenu(this);
    sortMenu->addAction(tr("File Name"), [=]() {
        mTopWindow->slotSetCurrentSortColumn(0);
    });
    sortMenu->addAction(tr("File Type"), [=]() {
        mTopWindow->slotSetCurrentSortColumn(1);
    });
    sortMenu->addAction(tr("File Size"), [=]() {
        mTopWindow->slotSetCurrentSortColumn(2);
    });
    sortMenu->addAction(tr("Modified Date"), [=]() {
        mTopWindow->slotSetCurrentSortColumn(3);
    });

    sortMenu->addSeparator();

    sortMenu->addAction(tr("Ascending"), [=]() {
        mTopWindow->slotSetCurrentSortOrder(Qt::AscendingOrder);
        mSortAction->setIcon(QIcon::fromTheme("view-sort-ascending-symbolic"));
    });
    sortMenu->addAction(tr("Descending"), [=] {
        mTopWindow->slotSetCurrentSortOrder(Qt::DescendingOrder);
        mSortAction->setIcon(QIcon::fromTheme("view-sort-descending-symbolic"));
    });

    mSortAction->setMenu(sortMenu);
    addAction(mSortAction);

    connect(mSortAction, &QAction::triggered, [=]() {
        auto point = this->widgetForAction(mSortAction)->geometry().bottomLeft();
        auto global_point = mapToGlobal(point);
        sortMenu->exec(global_point);
    });

    connect(sortMenu, &QMenu::aboutToShow, [=]() {
        for (auto action : sortMenu->actions()) {
            action->setCheckable(false);
            action->setChecked(false);
        }
        int column = mTopWindow->getCurrentSortColumn();
        int order = mTopWindow->getCurrentSortOrder();
        sortMenu->actions().at(column)->setCheckable(true);
        sortMenu->actions().at(column)->setChecked(true);
        sortMenu->actions().at(order + 5)->setCheckable(true);
        sortMenu->actions().at(order + 5)->setChecked(true);
    });
    addSeparator();

    //file operations
    QAction *copyAction = addAction(QIcon::fromTheme("edit-copy-symbolic"), tr("Copy"));
    copyAction->setShortcut(QKeySequence::Copy);

    QAction *pasteAction = addAction(QIcon::fromTheme("edit-paste-symbolic"), tr("Paste"));
    pasteAction->setShortcut(QKeySequence::Paste);

    QAction *cutAction = addAction(QIcon::fromTheme("edit-cut-symbolic"), tr("Cut"));
    cutAction->setShortcut(QKeySequence::Cut);

    QAction *trashAction = addAction(QIcon::fromTheme("edit-delete-symbolic"), tr("Trash"));
    //trashAction->setShortcut(QKeySequence::Delete);

    mFileOpActions<<copyAction<<pasteAction<<cutAction<<trashAction;

    addSeparator();

    //advance usage
    //...

    //separator widget

    //extension

    //other options?

    //trash
    mCleanTrashAction = addAction(QIcon::fromTheme("edit-clear-symbolic"), tr("Clean Trash"), [=]() {
        auto result = QMessageBox::question(nullptr, tr("Delete Permanently"), tr("Are you sure that you want to delete these files? "
                                            "Once you start a deletion, the files deleting will never be "
                                            "restored again."));
        if (result == QMessageBox::Yes) {
            auto uris = mTopWindow->getCurrentAllFileUris();
            qDebug()<<uris;
            FileOperationUtils::remove(uris);
        }
    });

    mRestoreAction = addAction(QIcon::fromTheme("view-refresh-symbolic"), tr("Restore"), [=]() {
        FileOperationUtils::restore(mTopWindow->getCurrentSelections());
    });

    mTrashActionsSperator = addSeparator();

    //connect signal
    connect(newWindowAction, &QAction::triggered, [=]() {
        FMWindow *newWindow = new FMWindow(mTopWindow->getLastNonSearchUri());
        newWindow->show();
        //FIXME: show when prepared
    });

    connect(newTabActon, &QAction::triggered, [=]() {
        QStringList l;
        l<<mTopWindow->getLastNonSearchUri();
        mTopWindow->slotAddNewTabs(l);
    });

    /*
    connect(viewCombox, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), [=](const QString &text){
        Q_EMIT this->optionRequest(SwitchView);
        //FIXME: i have to add interface to view proxy for view switch.
    });
    */

    connect(copyAction, &QAction::triggered, [=]() {
        if (!mTopWindow->getCurrentSelections().isEmpty())
            ClipbordUtils::setClipboardFiles(mTopWindow->getCurrentSelections(), false);
    });

    connect(pasteAction, &QAction::triggered, [=]() {
        if (ClipbordUtils::isClipboardHasFiles()) {
            //FIXME: how about duplicated copy?
            //FIXME: how to deal with a failed move?
            ClipbordUtils::pasteClipboardFiles(mTopWindow->getCurrentUri());
        }
    });
    connect(cutAction, &QAction::triggered, [=]() {
        if (!mTopWindow->getCurrentSelections().isEmpty()) {
            ClipbordUtils::setClipboardFiles(mTopWindow->getCurrentSelections(), true);
        }
    });
    connect(trashAction, &QAction::triggered, [=]() {
        if (!mTopWindow->getCurrentSelections().isEmpty()) {
            FileOperationUtils::trash(mTopWindow->getCurrentSelections(), true);
        }
    });

    QAction *optionAction = new QAction(QIcon::fromTheme("ukui-settings-app-symbolic", QIcon::fromTheme("settings-app-symbolic")), tr("Options"), nullptr);
    connect(optionAction, &QAction::triggered, this, [=]() {
        QMenu optionMenu;
        auto forbidThumbnail = optionMenu.addAction(tr("Forbid Thumbnail"), this, [=](bool checked) {
            GlobalSettings::getInstance()->setValue("do-not-thumbnail", checked);
        });
        forbidThumbnail->setCheckable(true);
        forbidThumbnail->setChecked(GlobalSettings::getInstance()->isExist("do-not-thumbnail")? GlobalSettings::getInstance()->getValue("do-not-thumbnail").toBool(): false);

        auto showHidden = optionMenu.addAction(tr("Show Hidden"), this, [=]() {
            mTopWindow->slotSetShowHidden();
        });
        showHidden->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
        showHidden->setCheckable(true);
        showHidden->setChecked(GlobalSettings::getInstance()->isExist("show-hidden")? GlobalSettings::getInstance()->getValue("show-hidden").toBool(): false);

        auto resident = optionMenu.addAction(tr("Resident in Backend"));
        resident->setToolTip(tr("Let the program still run after closing the last window. "
                                "This will reduce the time for the next launch, but it will "
                                "also consume resources in backend."));
        connect(resident, &QAction::triggered, this, [=](bool checked) {
            GlobalSettings::getInstance()->setValue("resident", checked);
            qApp->setQuitOnLastWindowClosed(!checked);
        });
        resident->setCheckable(true);
        resident->setChecked(GlobalSettings::getInstance()->isExist("resident")? GlobalSettings::getInstance()->getValue("resident").toBool(): false);

        optionMenu.addSeparator();

        auto help = optionMenu.addAction(QIcon::fromTheme("help-symbolic"), tr("&Help"), this, [=]() {
            QUrl url = QUrl("help:ubuntu-kylin-help/files", QUrl::TolerantMode);
            QDesktopServices::openUrl(url);
        });
        help->setShortcut(Qt::Key_F1);

        auto about = optionMenu.addAction(tr("&About..."), this, [=]() {
            QMessageBox::about(mTopWindow,
                               tr("Peony Qt"),
                               tr("Author:\n"
                                  "\tYue Lan <lanyue@kylinos.cn>\n"
                                  "\tMeihong He <hemeihong@kylinos.cn>\n"
                                  "\n"
                                  "Copyright (C): 2019, Tianjin KYLIN Information Technology Co., Ltd."));
        });
        about->setShortcut(Qt::CTRL + Qt::Key_F2);

        auto point = this->widgetForAction(optionAction)->geometry().bottomLeft();
        auto global_point = mapToGlobal(point);
        optionMenu.exec(global_point);
    });

    addAction(optionAction);

    //extension

    //trash
}

void ToolBar::updateLocation(const QString &uri)
{
    if (uri.isNull())
        return;

    bool isFileOpDisable = uri.startsWith("trash://") || uri.startsWith("search://")
                           || uri.startsWith("computer:///");
    for (auto action : mFileOpActions) {
        action->setEnabled(!isFileOpDisable);
        if (uri.startsWith("search://")) {
            if (action->text() == tr("Copy")) {
                action->setEnabled(true);
            }
        }
    }

    mViewFactoryModel->setDirectoryUri(uri);

    auto viewId = mTopWindow->getCurrentPage()->getView()->viewId();

    mViewAction->setIcon(mViewFactoryModel->iconFromViewId(viewId));
    mViewAction->setText(mTopWindow->getCurrentPageViewType());

    /*
    auto index = mViewFactoryModel->getIndexFromViewId(viewId);
    if (index.isValid())
        m_view_option_box->setCurrentIndex(index.row());
    else {
        m_view_option_box->setCurrentIndex(0);
    }
    */

    mCleanTrashAction->setVisible(uri.startsWith("trash://"));
    mRestoreAction->setVisible(uri.startsWith("trash://"));
    mTrashActionsSperator->setVisible(uri.startsWith("trash://"));
}

void ToolBar::updateStates()
{
    if (!mTopWindow)
        return;

    auto directory = mTopWindow->getCurrentUri();
    auto selection = mTopWindow->getCurrentSelections();

    if (directory.startsWith("trash://")) {
        auto files = mTopWindow->getCurrentAllFileUris();
        mCleanTrashAction->setEnabled(!files.isEmpty());
        mRestoreAction->setEnabled(!selection.isEmpty());
    }

    if (mTopWindow->getCurrentSortOrder() == Qt::AscendingOrder) {
        mSortAction->setIcon(QIcon::fromTheme("view-sort-ascending-symbolic"));
    } else {
        mSortAction->setIcon(QIcon::fromTheme("view-sort-descending-symbolic"));
    }
}

