#include "desktop-index-widget.h"

#include <QTimer>
#include <QStyle>
#include <QPainter>
#include <QTextEdit>
#include <QTextOption>
#include <QMouseEvent>
#include <QApplication>
#include <QStandardPaths>

#include "desktop-icon-view.h"
#include "desktop-icon-view-delegate.h"

#include <delegate/icon-view-delegate.h>

DesktopIndexWidget::DesktopIndexWidget(DesktopIconViewDelegate *delegate, const QStyleOptionViewItem &option, const QModelIndex &index, QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0, 0, 0, 0);
    mOption = option;

    mIndex = index;
    mDelegate = delegate;

    mCurrentFont = QApplication::font();

    updateItem();

    //FIXME: how to handle it in old version?
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    connect(qApp, &QApplication::fontChanged, this, [=]() {
        mDelegate->getView()->setIndexWidget(mIndex, nullptr);
    });
#endif

    auto view = mDelegate->getView();
    view->mRealDoEdit = false;
    view->mEditTriggerTimer.stop();
    view->mEditTriggerTimer.start();
}

DesktopIndexWidget::~DesktopIndexWidget()
{

}

void DesktopIndexWidget::updateItem()
{
    auto view = mDelegate->getView();
    mOption = view->viewOptions();
    mDelegate->initStyleOption(&mOption, mIndex);
    QSize size = mDelegate->sizeHint(mOption, mIndex);
    auto visualRect = mDelegate->getView()->visualRect(mIndex);
    move(visualRect.topLeft());
    setFixedWidth(visualRect.width());

    mOption.rect.setWidth(visualRect.width());

    int rawHeight = size.height();
    auto textSize = IconViewTextHelper::getTextSizeForIndex(mOption, mIndex, 2);
    int fixedHeight = 5 + mDelegate->getView()->iconSize().height() + 5 + textSize.height() + 5;
    if (fixedHeight < rawHeight)
        fixedHeight = rawHeight;

    mOption.text = mIndex.data().toString();
    //qDebug()<<mOption.text;
    //mOption.features.setFlag(QStyleOptionViewItem::WrapText);
    mOption.features |= QStyleOptionViewItem::WrapText;
    mOption.textElideMode = Qt::ElideNone;

    //mOption.rect.setHeight(9999);
    mOption.rect.moveTo(0, 0);

    //qDebug()<<mOption.rect;
    //auto font = view->getViewItemFont(&mOption);

    auto rawTextRect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &mOption, mOption.widget);
    auto iconSizeExcepted = mDelegate->getView()->iconSize();

    auto iconRect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &mOption, mOption.widget);

    auto y_delta = iconSizeExcepted.height() - iconRect.height();

    rawTextRect.setTop(iconRect.bottom() + y_delta + 5);
    rawTextRect.setHeight(9999);

    QFontMetrics fm(mCurrentFont);
    auto textRect = QApplication::style()->itemTextRect(fm, rawTextRect, Qt::AlignTop|Qt::AlignHCenter|Qt::TextWrapAnywhere, true, mOption.text);

    mTextRect = textRect;

    auto opt = mOption;
    setFixedHeight(fixedHeight);
}

void DesktopIndexWidget::paintEvent(QPaintEvent *e)
{
    auto visualRect = mDelegate->getView()->visualRect(mIndex);
    move(visualRect.topLeft());

    Q_UNUSED(e)
    QPainter p(this);
    auto bgColor = mOption.palette.highlight().color();
    p.save();
    p.setPen(Qt::transparent);
    bgColor.setAlpha(255*0.7);
    p.setBrush(bgColor);
    p.drawRoundedRect(this->rect(), 6, 6);
    p.restore();

    auto view = mDelegate->getView();
    //auto font = view->getViewItemFont(&mOption);

    auto opt = mOption;
    auto iconSizeExcepted = mDelegate->getView()->iconSize();
    auto iconRect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
    int y_delta = iconSizeExcepted.height() - iconRect.height();
    opt.rect.moveTo(opt.rect.x(), opt.rect.y() + y_delta);

    //setFixedHeight(opt.rect.height() + y_delta);

    // draw icon
    opt.text = nullptr;
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, &p, mDelegate->getView());

    p.save();
    p.translate(0, 5 + mDelegate->getView()->iconSize().height() + 5);

    // draw text shadow
    p.save();
    p.translate(1, 1);
    QColor shadow = Qt::black;
    shadow.setAlpha(127);
    p.setPen(shadow);
    IconViewTextHelper::paintText(&p, mOption, mIndex, 9999, 2, 0, false);

    p.restore();

    p.setPen(mOption.palette.highlightedText().color());
    IconViewTextHelper::paintText(&p, mOption, mIndex, 9999, 2, 0, false);

    p.restore();

    bgColor.setAlpha(255*0.8);
    p.setPen(bgColor);
    p.drawRoundedRect(this->rect().adjusted(0, 0, -1, -1), 6, 6);
}

void DesktopIndexWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        auto view = mDelegate->getView();
        view->mRealDoEdit = true;
        if (view->mEditTriggerTimer.isActive()) {
            if (view->mEditTriggerTimer.remainingTime() < 2250 && view->mEditTriggerTimer.remainingTime() > 0) {
                QTimer::singleShot(300, view, [=]() {
                    if (view->mRealDoEdit) {
                        bool special_index = false;
                        QString homeUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
                        special_index = (mIndex.data(Qt::UserRole).toString() == "computer:///" || mIndex.data(Qt::UserRole).toString() == "trash:///" || mIndex.data(Qt::UserRole).toString() == homeUri);
                        if (special_index) {
                            return ;
                        }

                        view->setIndexWidget(mIndex, nullptr);
                        view->edit(mIndex);
                    }
                });
            } else {
                view->mRealDoEdit = false;
                view->mEditTriggerTimer.stop();
            }
        } else {
            view->mRealDoEdit = false;
            view->mEditTriggerTimer.start();
        }
        event->accept();
        return;
//        if (m_edit_trigger.isActive()) {
//            qDebug()<<"IconViewIndexWidget::mousePressEvent: edit"<<e->type();
//            mDelegate->getView()->setIndexWidget(mIndex, nullptr);
//            mDelegate->getView()->edit(mIndex);
//            return;
//        }
    }
    QWidget::mousePressEvent(event);
}

void DesktopIndexWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    mDelegate->getView()->doubleClicked(mIndex);
    mDelegate->getView()->setIndexWidget(mIndex, nullptr);

    return;
}
