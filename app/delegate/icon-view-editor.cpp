#include "icon-view-editor.h"

#include <QPainter>
#include <QPolygon>
#include <QLineEdit>
#include <QPaintEvent>
#include <QAbstractItemView>

IconViewEditor::IconViewEditor(QWidget *parent) : QTextEdit(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
     mStyledEdit = new QLineEdit;
    setContentsMargins(0, 0, 0, 0);
    setAlignment(Qt::AlignCenter);

    setStyleSheet("padding: 0px;"
                  "background-color: white;");

    connect(this, &IconViewEditor::textChanged, this, &IconViewEditor::minimalAdjust);
}

IconViewEditor::~IconViewEditor()
{
     mStyledEdit->deleteLater();
}

void IconViewEditor::paintEvent(QPaintEvent *e)
{
    QPainter p(this->viewport());
    p.fillRect(this->viewport()->rect(), mStyledEdit->palette().base());
    QPen pen;
    pen.setWidth(2);
    pen.setColor(this->palette().highlight().color());
    QPolygon polygon = this->viewport()->rect();
    p.setPen(pen);
    p.drawPolygon(polygon);
    QTextEdit::paintEvent(e);
}

void IconViewEditor::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        Q_EMIT returnPressed();
        return;
    }
    QTextEdit::keyPressEvent(e);
}

void IconViewEditor::minimalAdjust()
{
    this->resize(QSize(document()->size().width(), document()->size().height() + 10));
}
