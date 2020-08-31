#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QTreeView>

class SideBar : public QTreeView
{
    Q_OBJECT
    friend class SideBarDelegate;
public:
    explicit SideBar (QWidget *parent = nullptr);
    QSize sizeHint () const override;

Q_SIGNALS:
    void updateWindowLocationRequest(const QString &uri, bool addHistory = true, bool forceUpdate = false);

protected:
    void paintEvent (QPaintEvent *e) override;
    QRect visualRect (const QModelIndex &index) const override;
    //int horizontalOffset() const override {return 100;}

    void dragEnterEvent (QDragEnterEvent *e) override;
    void dragMoveEvent (QDragMoveEvent *e) override;
};

#endif // SIDEBAR_H
