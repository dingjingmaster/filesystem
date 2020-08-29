#include "search-bar.h"

#include <QCompleter>
#include <QAction>
#include <QIcon>
#include <QWidget>
#include <QHeaderView>

#include <QDebug>

SearchBar::SearchBar(FMWindow *window, QWidget *parent) : QLineEdit(parent)
{
    m_top_window = window;
    init(window? true: false);
}

void SearchBar::init(bool hasTopWindow)
{
    setTextMargins(5, 0, 0, 0);
    setFixedWidth(175);

    setToolTip(tr("Input the search key of files you would like to find."));

    m_model = new QStandardItemModel(this);
    QCompleter *completer = new QCompleter(this);
    completer->setModel(m_model);
    completer->setMaxVisibleItems(10);

    //add two button in the completer, use QTableView
    m_table_view= new QTableView(this);
    m_table_view->setShowGrid(false);
    m_table_view->horizontalHeader()->setDefaultSectionSize(120);
    m_table_view->verticalHeader()->setDefaultSectionSize(6);
    m_table_view->horizontalHeader()->setVisible(false);
    m_table_view->verticalHeader()->setVisible(false);
    m_table_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //m_table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    initTableModel();

    completer->setPopup(m_table_view);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    this->setCompleter(completer);
    //end completer

    setLayoutDirection(Qt::LeftToRight);
    setPlaceholderText(tr("Input search key..."));
    QAction *searchAction = addAction(QIcon::fromTheme("edit-find-symbolic"), QLineEdit::TrailingPosition);
    //NOTE: we should not add a short cut for line edit,
    //because it might have some bad effect for other controls.
    //use returnPressed signal trigger the action instead.
    //searchAction->setShortcut(Qt::Key_Return);
    connect(this, &QLineEdit::returnPressed, searchAction, &QAction::trigger);
    connect(this, &QLineEdit::textChanged, this, &SearchBar::searchKeyChanged);
    connect(m_table_view, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onTableClicked(const QModelIndex &)));
    connect(searchAction, &QAction::triggered, this, &SearchBar::updateTableModel);
}

void SearchBar::focusInEvent(QFocusEvent *e)
{
    blockSignals(false);
    QLineEdit::focusInEvent(e);
    this->completer()->complete();
}

void SearchBar::focusOutEvent(QFocusEvent *e)
{
    blockSignals(true);
    QLineEdit::focusOutEvent(e);
    //this->clear();
}

void SearchBar::clearSearchRecord()
{
    m_model->clear();
    initTableModel();
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
    QTimer::singleShot(100, this, [=]() {
        m_table_view->setVisible(false);
    });
#else
    QTimer::singleShot(100, [=]() {
        m_table_view->setVisible(false);
    });
#endif
}

void SearchBar::initTableModel()
{
    QStandardItem *advance = new QStandardItem(tr("advance search"));
    QStandardItem *clear = new QStandardItem(tr("clear record"));
    QList<QStandardItem*> firstRow;
    firstRow<<advance<<clear;
    m_model->insertRow(0,firstRow);
    m_model->item(0)->setForeground(QBrush(QColor(10,10,255)));
    m_table_view->setMinimumHeight(25);
    m_table_view->setVisible(true);
}

void SearchBar::updateTableModel()
{
    if (!this->text().isEmpty()) {
        bool contained = false;
        for(int i=0; i<m_model->rowCount(); i++)
        {
            if(m_model->item(i)->text() == this->text())
            {
                contained = true;
                break;
            }
        }
        if (! contained)
        {
            QStandardItem *key = new QStandardItem(this->text());
            QList<QStandardItem*> row;
            m_model->insertRow(0, row<<key);
        }

        m_table_view->setMinimumHeight(m_model->rowCount() * 25);
        Q_EMIT this->searchRequest(this->text());
        this->clear();
        this->clearFocus();
    }
}

void SearchBar::onTableClicked(const QModelIndex &index)
{
    qDebug()<<"onTableClicked"<<index.row()<<m_model->rowCount()<<index.column()<<this->text();
    m_table_view->clearSelection();
    if (index.row() != m_model->rowCount()-1)
    {
        this->setText(m_model->item(index.row())->text());
        return;
    }
    if (index.column() == 0)
    {
        //clicked advance search
        m_top_window->slotAdvanceSearch();
        //qDebug()<<"show or hidden advance filter";
    }
    else if(index.column() == 1)
    {
        //clicked clear record
        clearSearchRecord();
    }
    this->setText("");
}

void SearchBar::hideTableView()
{
    m_table_view->setVisible(false);
}
