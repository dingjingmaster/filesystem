#include "icon-view-index-widget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QLabel>

#include <QApplication>
#include <QStyle>
#include <QTextDocument>
#include <QScrollBar>

#include <QMouseEvent>

#include "icon-view.h"
#include "file/file-info.h"
#include "model/file-item.h"
#include <delegate/icon-view-delegate.h>
#include <model/fileitem-proxy-filter-sort-model.h>

#include <delegate/icon-view-delegate.h>


IconViewIndexWidget::IconViewIndexWidget(const IconViewDelegate *delegate, const QStyleOptionViewItem &option, const QModelIndex &index, QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);

     mEditTrigger.setInterval(3000);
     mEditTrigger.setSingleShot(true);
#if QT_VERSION > QT_VERSION_CHECK(5, 12, 0)
    QTimer::singleShot(750, this, [=]() {
         mEditTrigger.start();
    });
#else
    QTimer::singleShot(750, this, [=]() {
         mEditTrigger.start();
    });
#endif
    //use QTextEdit to show full file name when select
     mEdit = new QTextEdit();

     mOption = option;
     mIndex = index;

     mDelegate = delegate;

     mDelegate->getView()->mRenameTimer->stop();
     mDelegate->getView()->mEditValid = false;
     mDelegate->getView()->mRenameTimer->start();

     mIsDragging =  mDelegate->getView()->isDraggingState();

    QSize size = delegate->sizeHint(option, index);
    setMinimumSize(size);

    //extra emblems
    auto  proxyModel = static_cast<FileItemProxyFilterSortModel*>(delegate->getView()->model());
    auto item = proxyModel->itemFromIndex(index);
    if (item) {
         mInfo = item->info();
    }

     mDelegate->initStyleOption(& mOption,  mIndex);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
     mOption.features.setFlag(QStyleOptionViewItem::WrapText);
#else
     mOption.features |= QStyleOptionViewItem::WrapText;
#endif
     mOption.textElideMode = Qt::ElideNone;

    auto opt =  mOption;
    opt.rect.moveTo(0, 0);

    auto iconExpectedSize =  mDelegate->getView()->iconSize();
    QRect iconRect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
    auto y_delta = iconExpectedSize.height() - iconRect.height();
    opt.rect.moveTo(opt.rect.x(), opt.rect.y() + y_delta);

     mOption = opt;

    adjustPos();
    auto textSize = IconViewTextHelper::getTextSizeForIndex(opt, index, 2);
    int fixedHeight = 5 + iconExpectedSize.height() + 5 + textSize.height() + 5;

    int y_bottom = option.rect.y() + fixedHeight + 20;
     bElideText = false;
    if ( y_bottom >  mDelegate->getView()->height() && opt.text.length() > ELIDE_TEXT_LENGTH)
    {
         bElideText = true;
        int  charWidth = opt.fontMetrics.averageCharWidth();
        opt.text = opt.fontMetrics.elidedText(opt.text, Qt::ElideRight, ELIDE_TEXT_LENGTH * charWidth);
        //recount size
        textSize = IconViewTextHelper::getTextSizeForIndex(opt, index, 2);
        fixedHeight = 5 + iconExpectedSize.height() + 5 + textSize.height() + 5;
    }

    if (fixedHeight >= option.rect.height())
        setFixedHeight(fixedHeight);
    else
        setFixedHeight(option.rect.height());

     mOption.rect.setHeight(fixedHeight - y_delta);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    connect(qApp, &QApplication::fontChanged, this, [=]() {
         mDelegate->getView()->setIndexWidget(mIndex, nullptr);
    });
#else
    //FIXME: handle font change.
#endif
}

IconViewIndexWidget::~IconViewIndexWidget()
{
    delete  mEdit;
}

void IconViewIndexWidget::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
    QPainter p(this);
    //p.fillRect(0, 0, 999, 999, Qt::red);

    adjustPos();
    auto opt =  mOption;
    auto rawRect =  mOption.rect;
    opt.rect = this->rect();
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem,
                                         &opt,
                                         &p,
                                         nullptr);

    opt.rect = rawRect;
    auto tmp = opt.text;
    opt.text = nullptr;
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, &p, opt.widget);
    if ( bElideText)
    {
        int  charWidth = opt.fontMetrics.averageCharWidth();
        tmp = opt.fontMetrics.elidedText(tmp, Qt::ElideRight, ELIDE_TEXT_LENGTH * charWidth);
    }
    opt.text = std::move(tmp);

    //auto textRectF = QRectF(0,  mDelegate->getView()->iconSize().height(), this->width(), this->height());
    p.save();

    p.setPen(opt.palette.highlightedText().color());
    p.translate(0,  mDelegate->getView()->iconSize().height() + 5);
    IconViewTextHelper::paintText(&p, opt,  mIndex, 9999, 2, 0);

//    p.translate(-1,  mDelegate->getView()->iconSize().height() + 13);
//    // mEdit->document()->drawContents(&p);
//    QTextOption textOption(Qt::AlignTop|Qt::AlignHCenter);
//    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
//    p.setFont(opt.font);
//    p.setPen(opt.palette.highlightedText().color());
//    p.drawText(QRect(1, 0, this->width() - 1, 9999), opt.text, textOption);
    p.restore();

    //extra emblems
    if (! mInfo.lock()) {
        return;
    }
    auto info =  mInfo.lock();
    //paint symbolic link emblems
    if (info->isSymbolLink()) {
        QIcon icon = QIcon::fromTheme("emblem-symbolic-link");
        //qDebug()<< "symbolic:" << info->symbolicIconName();
        icon.paint(&p, this->width() - 30, 10, 20, 20, Qt::AlignCenter);
    }

    //paint access emblems
    //NOTE: we can not query the file attribute in smb:///(samba) and network:///.
    if (!info->uri().startsWith("file:")) {
        return;
    }

    auto rect = this->rect();
    if (!info->canRead()) {
        QIcon icon = QIcon::fromTheme("emblem-unreadable");
        icon.paint(&p, rect.x() + 10, rect.y() + 10, 20, 20);
    } else if (!info->canWrite() && !info->canExecute()) {
        QIcon icon = QIcon::fromTheme("emblem-readonly");
        icon.paint(&p, rect.x() + 10, rect.y() + 10, 20, 20);
    }
}

void IconViewIndexWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        IconView *view =  mDelegate->getView();

        if (view->isDraggingState() ||  mIsDragging) {
            view-> mRenameTimer->stop();
            view-> mEditValid = false;
            return QWidget::mousePressEvent(e);
        }

        view-> mEditValid = true;
        if (view-> mRenameTimer->isActive()) {
            if (view-> mRenameTimer->remainingTime() < 2250 && view-> mRenameTimer->remainingTime() > 0) {
                view->slotRename();
            } else {
                view-> mEditValid = false;
                view-> mRenameTimer->stop();
            }
        } else {
            view-> mEditValid = false;
            view-> mRenameTimer->start();
        }
        e->accept();
        return;
//        if ( mEditTrigger.isActive()) {
//            qDebug()<<"IconViewIndexWidget::mousePressEvent: edit"<<e->type();
//             mDelegate->getView()->setIndexWidget( mIndex, nullptr);
//             mDelegate->getView()->edit( mIndex);
//            return;
//        }
    }
    QWidget::mousePressEvent(e);
}

void IconViewIndexWidget::mouseMoveEvent(QMouseEvent *e)
{
    QWidget::mouseMoveEvent(e);
     mIsDragging = true;
}

void IconViewIndexWidget::mouseReleaseEvent(QMouseEvent *e)
{
    QWidget::mouseReleaseEvent(e);
     mIsDragging = false;
}

void IconViewIndexWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
     mDelegate->getView()->doubleClicked( mIndex);
    return;
}

void IconViewIndexWidget::adjustPos()
{
    IconView *view =  mDelegate->getView();
    if ( mIndex.model() != view->model())
        return;

    auto visualRect = view->visualRect( mIndex);
    if (this->mapToParent(QPoint()) != visualRect.topLeft())
        this->move(visualRect.topLeft());
}

