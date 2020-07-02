#include "icon-view-delegate.h"

#include <QFont>
#include <QTimer>
#include <QLabel>
#include <QStyle>
#include <QPalette>
#include <QPainter>
#include <QFileInfo>
#include <QTextLayout>
#include <QTextCursor>
#include <QTextLayout>
#include <QPushButton>
#include <QTextOption>
#include <QApplication>
#include "clipbord-utils.h"
#include <view/icon-view.h>
#include <QGraphicsTextItem>
#include <model/file-item.h>
#include "icon-view-editor.h"
#include <model/file-item-model.h>
#include <file/file-rename-operation.h>
#include <file/file-operation-manager.h>
#include <view/icon-view-index-widget.h>
#include <model/fileitem-proxy-filter-sort-model.h>

#include <QTimer>

IconViewDelegate::IconViewDelegate(QObject *parent) : QStyledItemDelegate (parent)
{
     mStyledButton = new QPushButton;
}

IconViewDelegate::~IconViewDelegate()
{
    mStyledButton->deleteLater();
}

QSize IconViewDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    auto view = qobject_cast<IconView*>(this->parent());
    auto iconSize = view->iconSize();
    auto font = qApp->font();
    auto fm = QFontMetrics(font);
    int width = iconSize.width() + 41;
    int height = iconSize.height() + fm.ascent()*2 + 20;
    return QSize(width, height);
}

void IconViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    auto view = qobject_cast<IconView*>(this->parent());
    if (view->state() == IconView::DraggingState) {
        if (view->selectionModel()->selection().contains(index)) {
            painter->setOpacity(0.8);
        }
    }

    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    auto style = QApplication::style();

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, nullptr);

    if (ClipbordUtils::getClipedFilesParentUri() == view->getDirectoryUri()) {
        if (ClipbordUtils::isClipboardFilesBeCut()) {
            auto clipedUris = ClipbordUtils::getClipboardFilesUris();
            if (clipedUris.contains(index.data(FileItemModel::UriRole).toString())) {
                painter->setOpacity(0.5);
            }
        }
    }

    auto iconSizeExpected = view->iconSize();
    auto iconRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
    int y_delta = iconSizeExpected.height() - iconRect.height();
    opt.rect.setY(opt.rect.y() + y_delta);

    auto text = opt.text;
    opt.text = nullptr;
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    opt.text = text;
    painter->save();
    painter->translate(opt.rect.topLeft());
    painter->translate(0, iconRect.size().height() + 5);
    IconViewTextHelper::paintText(painter, opt, index, 9999, 2, 2);

    painter->restore();

    auto model = static_cast<FileItemProxyFilterSortModel*>(view->model());
    auto item = model->itemFromIndex(index);
    if (!item) {
        return;
    }
    auto info = item->info();
    auto rect = view->visualRect(index);

    bool useIndexWidget = false;
    if (view->selectedIndexes().count() == 1 && view->selectedIndexes().first() == index) {
        useIndexWidget = true;
        if (view->indexWidget(index)) {
        } else if (view->state() != IconView::DragSelectingState) {
            IconViewIndexWidget *indexWidget = new IconViewIndexWidget(this, option, index, getView());
            view->setIndexWidget(index, indexWidget);
        }
    }

    //paint symbolic link emblems
    if (info->isSymbolLink()) {
        QIcon icon = QIcon::fromTheme("emblem-symbolic-link");
        //qDebug()<<info->symbolicIconName();
        icon.paint(painter, rect.x() + rect.width() - 30, rect.y() + 10, 20, 20, Qt::AlignCenter);
    }

    //paint access emblems

    //NOTE: we can not query the file attribute in smb:///(samba) and network:///.
    if (info->uri().startsWith("file:")) {
        if (!info->canRead()) {
            QIcon icon = QIcon::fromTheme("emblem-unreadable");
            icon.paint(painter, rect.x() + 10, rect.y() + 10, 20, 20);
        } else if (!info->canWrite() && !info->canExecute()) {
            QIcon icon = QIcon::fromTheme("emblem-readonly");
            icon.paint(painter, rect.x() + 10, rect.y() + 10, 20, 20);
        }
        painter->restore();
        return;
    }

    painter->restore();

    //single selection, we have to repaint the emblems.

}

void IconViewDelegate::setCutFiles(const QModelIndexList &indexes)
{
    mCutIndexes = indexes;
}

QWidget *IconViewDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    auto edit = new IconViewEditor(parent);
    edit->setContentsMargins(0, 0, 0, 0);
    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setMinimumSize(sizeHint(option, index).width(), 54);

    edit->setText(index.data(Qt::DisplayRole).toString());
    edit->setAlignment(Qt::AlignCenter);
    //NOTE: if we directly call this method, there will be
    //nothing happen. add a very short delay will ensure that
    //the edit be resized.
    QTimer::singleShot(1, [=]() {
        edit->minimalAdjust();
    });

    connect(edit, &IconViewEditor::returnPressed, [=]() {
        this->setModelData(edit, nullptr, index);
        edit->deleteLater();
    });

    connect(edit, &QWidget::destroyed, this, [=]() {
        // NOTE: resort view after edit closed.
        // it's because if we not, the viewport might
        // not be updated in some cases.
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
        QTimer::singleShot(100, this, [=]() {
            auto model = qobject_cast<QSortFilterProxyModel*>(getView()->model());
            //fix rename file back to default sort order
            //model->sort(-1, Qt::SortOrder(getView()->getSortOrder()));
            model->sort(getView()->getSortType(), Qt::SortOrder(getView()->getSortOrder()));
        });
#else
        QTimer::singleShot(100, [=]() {
            auto model = qobject_cast<QSortFilterProxyModel*>(getView()->model());
            //fix rename file back to default sort order
            //model->sort(-1, Qt::SortOrder(getView()->getSortOrder()));
            model->sort(getView()->getSortType(), Qt::SortOrder(getView()->getSortOrder()));
        });
#endif
    });

    return edit;
}

void IconViewDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    IconViewEditor *edit = qobject_cast<IconViewEditor*>(editor);
    if (!edit)
        return;

    auto cursor = edit->textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    //qDebug()<<cursor.position();
    if (edit->toPlainText().contains(".") && !edit->toPlainText().startsWith(".")) {
        cursor.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor, 2);
        //qDebug()<<cursor.position();
    }
    //qDebug()<<cursor.anchor();
    edit->setTextCursor(cursor);
}

void IconViewDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    auto edit = qobject_cast<IconViewEditor*>(editor);
    if (!edit)
        return;

    auto opt = option;
    auto iconExpectedSize = getView()->iconSize();
    //auto iconRect = opt.widget->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
    //auto y_delta = iconExpectedSize.height() - iconRect.height();
    //edit->move(opt.rect.x(), opt.rect.y() + y_delta + 10);
    edit->move(opt.rect.x(), opt.rect.y() + iconExpectedSize.height());

    edit->resize(edit->document()->size().width(), edit->document()->size().height() + 10);
}

void IconViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    IconViewEditor *edit = qobject_cast<IconViewEditor*>(editor);
    if (!edit)
        return;
    auto newName = edit->toPlainText();
    auto oldName = index.data(Qt::DisplayRole).toString();
    QFileInfo info(index.data().toUrl().path());
    auto suffix = "." + info.suffix();
    if (newName.isNull())
        return;
    if (newName == "." || newName == ".." || newName.trimmed() == "")
        newName = "";
    if (newName.length() >0 && newName != oldName && newName != suffix) {
        auto fileOpMgr = FileOperationManager::getInstance();
        auto renameOp = new FileRenameOperation(index.data(FileItemModel::UriRole).toString(), newName);

        connect(renameOp, &FileRenameOperation::operationFinished, getView(), [=](){
            auto info = renameOp->getOperationInfo().get();
            auto uri = info->target();
            QTimer::singleShot(100, getView(), [=]() {
                getView()->setSelections(QStringList()<<uri);
                getView()->scrollToSelection(uri);
            });
        });

        fileOpMgr->slotStartOperation(renameOp, true);
    }
}

void IconViewDelegate::setIndexWidget(const QModelIndex &index, QWidget *widget) const
{
    auto view = qobject_cast<IconView*>(this->parent());
    view->setIndexWidget(index, widget);
}

IconView *IconViewDelegate::getView() const
{
    return qobject_cast<IconView*>(parent());
}

const QBrush IconViewDelegate::selectedBrush() const
{
    return  mStyledButton->palette().highlight();
}

QSize IconViewTextHelper::getTextSizeForIndex(const QStyleOptionViewItem &option, const QModelIndex &index, int horizalMargin, int maxLineCount)
{
    int fixedWidth = option.rect.width() - horizalMargin*2;
    QString text = option.text;
    QFont font = option.font;
    QFontMetrics fontMetrics = option.fontMetrics;
    int lineSpacing = fontMetrics.lineSpacing();
    int textHight = 0;

    QTextLayout textLayout(text, font);

    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    opt.setAlignment(Qt::AlignHCenter);

    textLayout.setTextOption(opt);
    textLayout.beginLayout();

    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(fixedWidth);
        textHight += lineSpacing;
    }

    textLayout.endLayout();
    if (maxLineCount > 0) {
        textHight = qMin(maxLineCount * lineSpacing, textHight);
    }

    return QSize(fixedWidth, textHight);
}

void IconViewTextHelper::paintText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index, int textMaxHeight, int horizalMargin, int maxLineCount, bool useSystemPalette)
{
    painter->save();
    painter->translate(horizalMargin, 0);

    if (useSystemPalette) {
        if (option.state.testFlag(QStyle::State_Selected))
            painter->setPen(option.palette.highlightedText().color());
        else
            painter->setPen(option.palette.text().color());
    }

    int lineCount = 0;

    QString text = option.text;
    QFont font = option.font;
    QFontMetrics fontMetrics = option.fontMetrics;
    int lineSpacing = fontMetrics.lineSpacing();

    QTextLayout textLayout(text, font);

    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    opt.setAlignment(Qt::AlignHCenter);

    textLayout.setTextOption(opt);
    textLayout.beginLayout();

    int width = option.rect.width() - 2*horizalMargin;

    int y = 0;
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(width);
        int nextLineY = y + lineSpacing;
        lineCount++;

        if (textMaxHeight >= nextLineY + lineSpacing && lineCount != maxLineCount) {
            line.draw(painter, QPoint(0, y));
            y = nextLineY;
        } else {
            QString lastLine = option.text.mid(line.textStart());
            QString elidedLastLine = fontMetrics.elidedText(lastLine, Qt::ElideRight, width);
            auto rect = QRect(horizalMargin, y /*+ fontMetrics.ascent()*/, width, textMaxHeight);
            //opt.setWrapMode(QTextOption::NoWrap);
            opt.setWrapMode(QTextOption::NoWrap);
            painter->drawText(rect, elidedLastLine, opt);
            //painter->drawText(QPoint(0, y + fontMetrics.ascent()), elidedLastLine);
            line = textLayout.createLine();
            break;
        }
    }
    textLayout.endLayout();

    painter->restore();
}
