#include "list-view-delegate.h"

#include "list-view.h"

#include <QTimer>

#include <QPainter>
#include <QKeyEvent>
#include <QPushButton>
#include <QApplication>
#include <model/file-item-model.h>
#include <file/file-rename-operation.h>
#include <file/file-operation-manager.h>


ListViewDelegate::ListViewDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    mStyledButton = new QPushButton;
}

ListViewDelegate::~ListViewDelegate()
{
    mStyledButton->deleteLater();
}

void ListViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.displayAlignment = Qt::Alignment(Qt::AlignLeft|Qt::AlignVCenter);
    return QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter);
}

QWidget *ListViewDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    TextEdit *edit = new TextEdit(parent);
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setWordWrapMode(QTextOption::NoWrap);

    QTimer::singleShot(1, [=]() {
        this->updateEditorGeometry(edit, option, index);
    });

    connect(edit, &TextEdit::textChanged, [=]() {
        updateEditorGeometry(edit, option, index);
    });

    connect(edit, &TextEdit::finishEditRequest, [=]() {
        setModelData(edit, nullptr, index);
        edit->deleteLater();
    });

    return edit;
}

void ListViewDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TextEdit *edit = qobject_cast<TextEdit *>(editor);
    if (!edit)
        return;

    edit->setText(index.data(Qt::DisplayRole).toString());
    auto cursor = edit->textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    if (edit->toPlainText().contains(".") && !edit->toPlainText().startsWith(".")) {
        cursor.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor, 2);
    }
    edit->setTextCursor(cursor);
}

void ListViewDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    TextEdit *edit = qobject_cast<TextEdit*>(editor);
    edit->setFixedHeight(editor->height());
    edit->resize(edit->document()->size().width(), -1);
}

void ListViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    TextEdit *edit = qobject_cast<TextEdit*>(editor);
    if (!edit)
        return;

    auto text = edit->toPlainText();
    if (text.isEmpty())
        return;
    if (text == index.data(Qt::DisplayRole).toString())
        return;
    if (text == "." || text == ".." || text.trimmed() == "")
        return;

    auto view = qobject_cast<ListView *>(parent());

    auto fileOpMgr = FileOperationManager::getInstance();
    auto renameOp = new FileRenameOperation(index.data(FileItemModel::UriRole).toString(), text);

    connect(renameOp, &FileRenameOperation::operationFinished, view, [=](){
        auto info = renameOp->getOperationInfo().get();
        auto uri = info->target();
        QTimer::singleShot(100, view, [=](){
            view->setSelections(QStringList()<<uri);
            view->scrollToSelection(uri);
        });
    });

    fileOpMgr->slotStartOperation(renameOp, true);
}

TextEdit::TextEdit(QWidget *parent) : QTextEdit (parent)
{

}

void TextEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        Q_EMIT finishEditRequest();
        return;
    }
    return QTextEdit::keyPressEvent(e);
}
