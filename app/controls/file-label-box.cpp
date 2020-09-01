#include "file-label-box.h"

#include "model/filelabel-model.h"
#include "label-box-delegate.h"

#include <QMenu>

#include <QColorDialog>
#include <QMouseEvent>

#include <QApplication>
#include <QDebug>

static LabelBoxStyle *global_instance = nullptr;

FileLabelBox::FileLabelBox(QWidget *parent) : QListView(parent)
{
    //setItemDelegate(new LabelBoxDelegate(this));
    setStyle(LabelBoxStyle::getStyle());
    viewport()->setStyle(LabelBoxStyle::getStyle());
    setModel(FileLabelModel::getGlobalModel());

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QWidget::customContextMenuRequested, this, [=](const QPoint &pos) {
        auto index = indexAt(pos);
        bool labelRemovable = false;
        QMenu menu;
        if (index.isValid()) {
            auto item = FileLabelModel::getGlobalModel()->itemFormIndex(index);
            int id = item->id();
            if (id > TOTAL_DEFAULT_COLOR)
                labelRemovable = true;

            menu.addAction(tr("Rename"), [=]() {
                //FIXME: edit
                edit(index);
            });

            menu.addAction(tr("Edit Color"), [=]() {
                QColorDialog d;
                if (d.exec()) {
                    auto color = d.selectedColor();
                    item->setColor(color);
                }
            });

            auto a = menu.addAction(tr("Delete"), [=]() {
                FileLabelModel::getGlobalModel()->removeLabel(id);
            });
            a->setEnabled(labelRemovable);
        } else {
            menu.addAction(tr("Create New Label"), [=]() {
                QColorDialog dialog;
                if (dialog.exec()) {
                    auto color = dialog.selectedColor();
                    auto name = color.name();
                    FileLabelModel::getGlobalModel()->addLabel(name, color);
                }
            });
        }
        menu.exec(QCursor::pos());
    });
}

QSize FileLabelBox::sizeHint() const
{
    auto w = this->topLevelWidget()->width();
    auto size = QListView::sizeHint();
    size.setWidth(w/5);
    return size;
}

void FileLabelBox::mousePressEvent(QMouseEvent *e)
{
    QModelIndex index = indexAt(e->pos());
    //qDebug() << "mousePressEvent:"<<e->pos() <<index.isValid() <<index.data() <<e->type();
    if (!index.isValid() && e->button() == Qt::LeftButton)
    {
        this->clearSelection();
        Q_EMIT leftClickOnBlank();
    }
    else {
        QListView::mousePressEvent(e);
    }
}

void FileLabelBox::paintEvent(QPaintEvent *e)
{
    QListView::paintEvent(e);
}

//LabelBoxStyle
LabelBoxStyle *LabelBoxStyle::getStyle()
{
    if (!global_instance) {
        global_instance = new LabelBoxStyle;
    }
    return global_instance;
}

void LabelBoxStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    return QApplication::style()->drawPrimitive(element, option, painter, widget);
}

