#ifndef ICONVIEWINDEXWIDGET_H
#define ICONVIEWINDEXWIDGET_H
#include <QWidget>

#include <QModelIndex>
#include <QStyleOptionViewItem>
#include <QTextEdit>
#include <QTimer>

#include <memory>

class FileInfo;
class IconViewDelegate;

class IconViewIndexWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IconViewIndexWidget(const IconViewDelegate *delegate, const QStyleOptionViewItem &option, const QModelIndex &index, QWidget *parent = nullptr);
    ~IconViewIndexWidget() override;

protected:
    void adjustPos();
    void paintEvent(QPaintEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QTextEdit *mEdit;
    QModelIndex mIndex;
    QTimer mEditTrigger;
    bool bElideText = false;
    bool mIsDragging = false;
    QStyleOptionViewItem mOption;
    std::weak_ptr<FileInfo> mInfo;
    const int ELIDE_TEXT_LENGTH = 32;
    const IconViewDelegate *mDelegate;
};

#endif // ICONVIEWINDEXWIDGET_H
