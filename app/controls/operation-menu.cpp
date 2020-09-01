#include "operation-menu.h"

#include "main/fm-window.h"

#include "window/main-window.h"
#include "operation-menu.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QWidgetAction>

#include "clipbord-utils.h"
#include "global-settings.h"
#include "file-operation-utils.h"
#include <QApplication>

OperationMenu::OperationMenu(MainWindow *window, QWidget *parent) : QMenu(parent)
{
    mWindow = window;

    connect(this, &QMenu::aboutToShow, this, &OperationMenu::updateMenu);

    //FIXME: implement all actions.

    auto editWidgetContainer = new QWidgetAction(this);
    auto editWidget = new OperationMenuEditWidget(window, this);
    mEditWidget = editWidget;
    editWidgetContainer->setDefaultWidget(editWidget);
    addAction(editWidgetContainer);

    connect(mEditWidget, &OperationMenuEditWidget::operationAccepted, this, &QMenu::hide);

    addSeparator();

    //addAction(tr("Conditional Filter"));
//    auto advanceSearch = addAction(tr("Advance Search"), this, [=]()
//    {
//       mWindow->advanceSearch();
//    });

//    addSeparator();

    auto keepAllow = addAction(tr("Keep Allow"), this, [=](bool checked) {
        mWindow->setWindowFlags(Qt::WindowStaysOnTopHint|mWindow->windowFlags());
        mWindow->show();
    });
    keepAllow->setCheckable(true);

    auto showHidden = addAction(tr("Show Hidden"), this, [=](bool checked) {
        //window set show hidden
        mWindow->slotSetShowHidden();
    });
    mShowHidden = showHidden;
    showHidden->setCheckable(true);

    auto forbidThumbnailing = addAction(tr("Forbid thumbnailing"), this, [=](bool checked) {
        //FIXME:
        GlobalSettings::getInstance()->setValue("do-not-thumbnail", checked);
    });
    mForbidThumbnailing = forbidThumbnailing;
    forbidThumbnailing->setCheckable(true);
    forbidThumbnailing->setChecked(GlobalSettings::getInstance()->getValue("do-not-thumbnail").toBool());

    auto residentInBackend = addAction(tr("Resident in Backend"), this, [=](bool checked) {
        //FIXME:
        GlobalSettings::getInstance()->setValue("resident", checked);
        qApp->setQuitOnLastWindowClosed(!checked);
    });
    mResidentInBackend = residentInBackend;
    residentInBackend->setCheckable(true);
    residentInBackend->setChecked(GlobalSettings::getInstance()->getValue("resident").toBool());

    addSeparator();
}

void OperationMenu::updateMenu()
{
    mShowHidden->setChecked(GlobalSettings::getInstance()->isExist("show-hidden")?
                              GlobalSettings::getInstance()->getValue("show-hidden").toBool():
                              false);

    mForbidThumbnailing->setChecked(GlobalSettings::getInstance()->isExist("do-not-thumbnail")?
                                      GlobalSettings::getInstance()->getValue("do-not-thumbnail").toBool():
                                      false);

    mResidentInBackend->setChecked(GlobalSettings::getInstance()->isExist("resident")?
                                      GlobalSettings::getInstance()->getValue("resident").toBool():
                                      false);

    //get window current directory and selections, then update ohter actions.
    mEditWidget->updateActions(mWindow->getCurrentUri(), mWindow->getCurrentSelections());
}

OperationMenuEditWidget::OperationMenuEditWidget(MainWindow *window, QWidget *parent) : QWidget(parent)
{
    auto vbox = new QVBoxLayout;
    setLayout(vbox);

    auto title = new QLabel(this);
    title->setText(tr("Edit"));
    title->setAlignment(Qt::AlignCenter);
    vbox->addWidget(title);

    auto hbox = new QHBoxLayout;
    auto copy = new QToolButton(this);
    mCopy = copy;
    copy->setFixedSize(QSize(40, 40));
    copy->setIcon(QIcon::fromTheme("edit-copy-symbolic"));
    copy->setIconSize(QSize(16, 16));
    copy->setAutoRaise(false);
    hbox->addWidget(copy);

    auto paste = new QToolButton(this);
    mPaste = paste;
    paste->setFixedSize(QSize(40, 40));
    paste->setIcon(QIcon::fromTheme("edit-paste-symbolic"));
    paste->setIconSize(QSize(16, 16));
    paste->setAutoRaise(false);
    hbox->addWidget(paste);

    auto cut = new QToolButton(this);
    mCut = cut;
    cut->setFixedSize(QSize(40, 40));
    cut->setIcon(QIcon::fromTheme("edit-cut-symbolic"));
    cut->setIconSize(QSize(16, 16));
    cut->setAutoRaise(false);
    hbox->addWidget(cut);

    auto trash = new QToolButton(this);
    mTrash = trash;
    trash->setFixedSize(QSize(40, 40));
    trash->setIcon(QIcon::fromTheme("ukui-user-trash"));
    trash->setIconSize(QSize(16, 16));
    trash->setAutoRaise(false);
    hbox->addWidget(trash);

    vbox->addLayout(hbox);

    connect(mCopy, &QToolButton::clicked, this, [=]() {
        ClipbordUtils::setClipboardFiles(window->getCurrentSelections(), false);
        Q_EMIT operationAccepted();
    });

    connect(mCut, &QToolButton::clicked, this, [=]() {
        ClipbordUtils::setClipboardFiles(window->getCurrentSelections(), true);
        Q_EMIT operationAccepted();
    });

    connect(mPaste, &QToolButton::clicked, this, [=]() {
        ClipbordUtils::pasteClipboardFiles(window->getCurrentUri());
        Q_EMIT operationAccepted();
    });

    connect(mTrash, &QToolButton::clicked, this, [=]() {
        if (window->getCurrentUri() == "trash:///") {
            FileOperationUtils::executeRemoveActionWithDialog(window->getCurrentSelections());
        } else {
            FileOperationUtils::trash(window->getCurrentSelections(), true);
        }
        Q_EMIT operationAccepted();
    });

    copy->setProperty("useIconHighlightEffect", true);
    copy->setProperty("iconHighlightEffectMode", 1);
    copy->setProperty("fillIconSymbolicColor", true);
    paste->setProperty("useIconHighlightEffect", true);
    paste->setProperty("iconHighlightEffectMode", 1);
    paste->setProperty("fillIconSymbolicColor", true);
    cut->setProperty("useIconHighlightEffect", true);
    cut->setProperty("iconHighlightEffectMode", 1);
    cut->setProperty("fillIconSymbolicColor", true);
    trash->setProperty("useIconHighlightEffect", true);
    trash->setProperty("iconHighlightEffectMode", 1);
    trash->setProperty("fillIconSymbolicColor", true);
}

void OperationMenuEditWidget::updateActions(const QString &currentDirUri, const QStringList &selections)
{
    //FIXME:
    bool isSelectionEmpty = selections.isEmpty();
    mCopy->setEnabled(!isSelectionEmpty);
    mCut->setEnabled(!isSelectionEmpty);
    mTrash->setEnabled(!isSelectionEmpty);

    if (isSelectionEmpty) {
        bool isClipboradHasFile = ClipbordUtils::isClipboardHasFiles();
        mPaste->setEnabled(isClipboradHasFile);
    } else {
        mPaste->setEnabled(false);
    }
}
