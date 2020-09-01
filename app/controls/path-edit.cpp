#include "path-edit.h"

#include "model/pathbar-model.h"
#include "model/path-complete.h"

#include <QDebug>
#include <QAction>
#include <QKeyEvent>


PathEdit::PathEdit(QWidget *parent) : QLineEdit(parent)
{
    setFocusPolicy(Qt::ClickFocus);

    mModel = new PathBarModel(this);
    mCompleter = new PathComplete(this);
    mCompleter->setModel(mModel);
    mCompleter->setCaseSensitivity(Qt::CaseInsensitive);

    setLayoutDirection(Qt::LeftToRight);

    QAction *goToAction = new QAction(QIcon::fromTheme("forward"), tr("Go To"), this);
    addAction(goToAction, QLineEdit::TrailingPosition);

    connect(goToAction, &QAction::triggered, this, &QLineEdit::returnPressed);

    setCompleter(mCompleter);

    connect(this, &QLineEdit::returnPressed, [=] {
        if (this->text().isEmpty()) {
            this->setText(mLastUri);
            this->editCancelled();
            return;
        } else {
            qDebug()<<"change dir request"<<this->text();
            Q_EMIT this->uriChangeRequest(this->text());
            //NOTE: we have send the signal for location change.
            //so we can use editCancelled hide the path edit.
            this->editCancelled();
        }
    });
}

void PathEdit::setUri(const QString &uri)
{
    mLastUri = uri;
    setText(uri);
}

void PathEdit::focusOutEvent(QFocusEvent *e)
{
    QLineEdit::focusOutEvent(e);
    if (! mRightClick)
        Q_EMIT editCancelled();
}

void PathEdit::focusInEvent(QFocusEvent *e)
{
    QLineEdit::focusInEvent(e);
    mModel->setRootUri(this->text());
    mCompleter->complete();
}

void PathEdit::keyPressEvent(QKeyEvent *e)
{
    QLineEdit::keyPressEvent(e);
    if (e->key() == Qt::Key_Escape) {
        Q_EMIT editCancelled();
    }
}

void PathEdit::mousePressEvent(QMouseEvent *e)
{
    QLineEdit::mousePressEvent(e);
    //qDebug() << "mousePressEvent"<<e->button();
    if (e->button() == Qt::RightButton)
        mRightClick = true;
    else
        mRightClick = false;
}
