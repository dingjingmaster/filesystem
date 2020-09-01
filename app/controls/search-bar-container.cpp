#include "search-bar-container.h"

#include <QDebug>
#include <QAction>
#include <QCompleter>
#include <QStringListModel>

SearchBarContainer::SearchBarContainer(QWidget *parent): QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    this->setLayout(layout);
    mLayout = layout;
    layout->setContentsMargins(0,0,0,0);

    QComboBox *filter = new QComboBox(this);
    mFilterBox = filter;
    filter->setToolTip(tr("Choose File Type"));
    auto model = new QStringListModel(this);
    model->setStringList(mFileTypeList);
    filter->setModel(model);
    filter->setFixedWidth(80);
    filter->setFixedHeight(parent->height());

    QLineEdit *edit = new QLineEdit(this);
    mSearchBox = edit;
    edit->setFixedHeight(parent->height());

    layout->addWidget(filter, Qt::AlignLeft);
    layout->addWidget(edit, Qt::AlignLeft);

    //search history
    mModel = new QStringListModel(mSearchBox);
    QCompleter *completer = new QCompleter(mSearchBox);
    completer->setModel(mModel);
    completer->setMaxVisibleItems(10);

    auto m_list = mModel->stringList();
    m_list.prepend(tr("Clear"));
    mModel->setStringList(m_list);
    mListView = new QListView(mSearchBox);
    mListView->setModel(mModel);
    completer->setPopup(mListView);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    mSearchBox->setCompleter(completer);

    connect(mSearchBox, &QLineEdit::returnPressed, [=]()
    {
        if (! mSearchBox->text().isEmpty())
        {
            auto l = mModel->stringList();
            if (! l.contains(mSearchBox->text()))
                l.prepend(mSearchBox->text());

            mModel->setStringList(l);
        }
        Q_EMIT this->returnPressed();
    });
    connect(mFilterBox, &QComboBox::currentTextChanged, [=]()
    {
        Q_EMIT this->filterUpdate(mFilterBox->currentIndex());
    });

    QAction *searchAction = mSearchBox->addAction(QIcon::fromTheme("go-down"), QLineEdit::TrailingPosition);
    connect(searchAction, &QAction::triggered, this, [=]() {
        //qDebug() << "triggered search history!";
        mSearchBox->completer()->complete();
    });

    connect(mListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onTableClicked(const QModelIndex &)));
}

QSize SearchBarContainer::sizeHint() const
{
    return this->topLevelWidget()->sizeHint();
}

void SearchBarContainer::onTableClicked(const QModelIndex &index)
{
    //qDebug() << "onTableClicked:" <<index.data().toInt() <<mModel->rowCount();
    if (index.row() != mModel->rowCount()-1)
    {
        mSearchBox->setText(index.data().toString());
        return;
    }

    auto l = mModel->stringList();
    l.clear();
    l.prepend(tr("Clear"));
    mModel->setStringList(l);
    mSearchBox->setText("");
}
