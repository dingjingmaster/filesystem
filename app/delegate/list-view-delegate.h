#ifndef LISTVIEWDELEGATE_H
#define LISTVIEWDELEGATE_H

#include <QTextEdit>
#include <QStyledItemDelegate>

class TextEdit;
class QPushButton;

class ListViewDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ListViewDelegate(QObject *parent = nullptr);
    ~ListViewDelegate() override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QPushButton *mStyledButton;
};

class TextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit TextEdit(QWidget *parent = nullptr);

Q_SIGNALS:
    void finishEditRequest();

protected:
    void keyPressEvent(QKeyEvent *e);
};


#endif // LISTVIEWDELEGATE_H
