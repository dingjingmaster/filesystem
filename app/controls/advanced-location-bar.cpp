#include "advanced-location-bar.h"

#include "path-edit.h"
#include "location-bar.h"
#include "search-bar-container.h"
#include "vfs/search-vfs-uri-parser.h"

#include <QDebug>
#include <QStackedLayout>

AdvancedLocationBar::AdvancedLocationBar(QWidget *parent) : QWidget(parent)
{
    QStackedLayout *layout = new QStackedLayout(this);
    mLayout = layout;

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->setSizeConstraint(QLayout::SetDefaultConstraint);
    mBar = new LocationBar(this);
    mEdit = new PathEdit(this);
    mSearchBar = new SearchBarContainer(this);
    //bar->connect(bar, &Peony::LocationBar::groupChangedRequest, bar, &Peony::LocationBar::setRootUri);
    mBar->connect(mBar, &LocationBar::blankClicked, [=]() {
        layout->setCurrentWidget(mEdit);
        mEdit->setFocus();
        mEdit->setUri(mBar->getCurentUri());
    });

    mEdit->connect(mEdit, &PathEdit::uriChangeRequest, [=](const QString uri) {
        if (mText == uri) {
            Q_EMIT this->refreshRequest();
            return;
        }
        mBar->setRootUri(uri);
        layout->setCurrentWidget(mBar);
        Q_EMIT this->updateWindowLocationRequest(uri);
        mText = mEdit->text();
        if (! mText.startsWith("search://"))
            mLastNonSearchPath = mText;
    });

    mBar->connect(mBar, &LocationBar::groupChangedRequest, [=](const QString &uri) {
        if (mText == uri) {
            Q_EMIT this->refreshRequest();
            return;
        }
        Q_EMIT this->updateWindowLocationRequest(uri);
        mText = uri;
        if (! uri.startsWith("search://"))
            mLastNonSearchPath = uri;
    });

    mEdit->connect(mEdit, &PathEdit::editCancelled, [=]() {
        layout->setCurrentWidget(mBar);
    });

    mSearchBar->connect(mSearchBar, &SearchBarContainer::returnPressed, [=]() {
        //qDebug() << "start search" << mSearchBar->text();
        auto key = mSearchBar->text();
        auto targetUri = SearchVFSUriParser::parseSearchKey(mLastNonSearchPath, key);
        Q_EMIT this->updateWindowLocationRequest(targetUri);

    });

    mSearchBar->connect(mSearchBar, &SearchBarContainer::filterUpdate, [=](const int &index)
    {
        Q_EMIT this->updateFileTypeFilter(index);
    });


    layout->addWidget(mBar);
    layout->addWidget(mEdit);
    layout->addWidget(mSearchBar);

    setLayout(layout);
    setFixedHeight(mEdit->height());
}

void AdvancedLocationBar::updateLocation(const QString &uri)
{
    //qDebug() << "AdvancedLocationBar updateLocation:"<<uri;
    mBar->setRootUri(uri);
    mEdit->setUri(uri);
    mText = uri;
    if (! uri.startsWith("search://"))
        mLastNonSearchPath = uri;
    Q_EMIT this->refreshRequest();
}

bool AdvancedLocationBar::isEditing()
{
    return mEdit->isVisible();
}

void AdvancedLocationBar::startEdit()
{
    mEdit->setVisible(true);
    mLayout->setCurrentWidget(mEdit);
    mEdit->setFocus();
    mEdit->setUri(mBar->getCurentUri());
}

void AdvancedLocationBar::finishEdit()
{
    Q_EMIT mEdit->returnPressed();
}

void AdvancedLocationBar::switchEditMode(bool bSearchMode)
{
    if (bSearchMode)
    {
        mEdit->setVisible(false);
        mLayout->setCurrentWidget(mSearchBar);
        mSearchBar->setPlaceholderText(tr("Search Content..."));
        mSearchBar->setFocus();
    }
    else
    {
        mLayout->setCurrentWidget(mBar);
    }
}
