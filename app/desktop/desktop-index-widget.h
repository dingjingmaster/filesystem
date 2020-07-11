#ifndef DESKTOPINDEXWIDGET_H
#define DESKTOPINDEXWIDGET_H

#include <QStyleOptionViewItem>
#include <QWidget>

class DesktopIconViewDelegate;

class DesktopIndexWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DesktopIndexWidget(DesktopIconViewDelegate *delegate, const QStyleOptionViewItem &option, const QModelIndex &index, QWidget *parent = nullptr);
    ~DesktopIndexWidget();

protected:
    void updateItem();
    void paintEvent(QPaintEvent *e);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

private:
    QRect mTextRect;
    QModelIndex mIndex;
    QFont mCurrentFont;
    QStyleOptionViewItem mOption;
    const DesktopIconViewDelegate *mDelegate;
};

#endif // DESKTOPINDEXWIDGET_H
