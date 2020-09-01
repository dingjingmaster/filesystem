#include "location-bar.h"

#include "file-utils.h"
#include "model/pathbar-model.h"
#include "vfs/search-vfs-uri-parser.h"

#include <QUrl>
#include <QMenu>
#include <QDebug>
#include <QPainter>
#include <QLineEdit>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QStyleOptionFrame>
#include <QStyleOptionFocusRect>

LocationBar::LocationBar(QWidget *parent) : QToolBar(parent)
{
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);

    setToolTip(tr("click the blank area for edit"));

    setStyleSheet("padding-right: 15;"
                  "margin-left: 2");
    mStyledEdit = new QLineEdit;
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setIconSize(QSize(16, 16));
    qDebug()<<sizePolicy();
    //connect(this, &LocationBar::groupChangedRequest, this, &LocationBar::setRootUri);
}

LocationBar::~LocationBar()
{
    mStyledEdit->deleteLater();
}

void LocationBar::setRootUri(const QString &uri)
{
    mCurrentUri = uri;
    for (auto action : actions()) {
        removeAction(action);
    }

    if (mCurrentUri.startsWith("search://")) {
        QString nameRegexp = SearchVFSUriParser::getSearchUriNameRegexp(mCurrentUri);
        QString targetDirectory = SearchVFSUriParser::getSearchUriTargetDirectory(mCurrentUri);
        addAction(QIcon::fromTheme("edit-find-symbolic"), tr("Search \"%1\" in \"%2\"").arg(nameRegexp).arg(targetDirectory));
        return;
    }

    QStringList uris;
    QString tmp = uri;
    while (!tmp.isEmpty()) {
        uris.prepend(tmp);
        QUrl url = tmp;
        if (FileUtils::isMountRoot(tmp))
            break;

        if (url.path() == QStandardPaths::writableLocation(QStandardPaths::HomeLocation)) {
            break;
        }
        tmp = FileUtils::getParentUri(tmp);
    }

    for (auto uri : uris) {
        //addButton(uri, uri != uris.last());
        addButton(uri, uris.first() == uri);
    }
}

void LocationBar::addButton(const QString &uri, bool setIcon, bool setMenu)
{
    QAction *action = new QAction(this);
    QUrl url = uri;
    auto parent = FileUtils::getParentUri(uri);
    if (setIcon) {
        QIcon icon = QIcon::fromTheme(FileUtils::getFileIconName(uri), QIcon::fromTheme("folder"));
        action->setIcon(icon);
    }

    if (!url.fileName().isEmpty()) {
        if (FileUtils::getParentUri(uri).isNull()) {
            setMenu = false;
        }
        action->setText(url.fileName());
    } else {
        if (uri == "file:///") {
            auto text = FileUtils::getFileDisplayName("computer:///root.link");
            if (text.isNull()) {
                text = tr("File System");
            }
            action->setText(text);
        } else {
            action->setText(FileUtils::getFileDisplayName(uri));
        }
    }

    connect(action, &QAction::triggered, [=]() {
        //this->setRootUri(uri);
        Q_EMIT this->groupChangedRequest(uri);
    });

    if (setMenu) {
        PathBarModel m;
        m.setRootUri(uri);
        m.sort(0);

        auto suburis = m.stringList();
        if (!suburis.isEmpty()) {
            QMenu *menu = new QMenu(this);
            QList<QAction *> actions;
            for (auto uri : suburis) {
                QUrl url = uri;
                QString tmp = uri;
                QAction *action = new QAction(url.fileName(), this);
                actions<<action;
                connect(action, &QAction::triggered, [=]() {
                    Q_EMIT groupChangedRequest(tmp);
                });
            }
            menu->addActions(actions);

            action->setMenu(menu);
        }
    }

    addAction(action);
}

void LocationBar::mousePressEvent(QMouseEvent *e)
{
    //eat this event.
    //QToolBar::mousePressEvent(e);
    qDebug()<<"black clicked";
    if (e->button() == Qt::LeftButton) {
        Q_EMIT blankClicked();
    }
}

void LocationBar::paintEvent(QPaintEvent *e)
{
    //QToolBar::paintEvent(e);

    QPainter p(this);
    QStyleOptionFocusRect opt;
    opt.initFrom(this);

    QStyleOptionFrame fopt;
    fopt.initFrom(this);
    fopt.state |= QStyle::State_HasFocus;
    //fopt.state.setFlag(QStyle::State_HasFocus);
    fopt.rect.adjust(-2, 0, 0, 0);
    if (!(opt.state & QStyle::State_MouseOver))
        fopt.palette.setColor(QPalette::Highlight, fopt.palette.dark().color());

    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &fopt, &p, this);

    style()->drawControl(QStyle::CE_ToolBar, &opt, &p, this);
}
