#ifndef SIDEBARDELEGATE_H
#define SIDEBARDELEGATE_H

#include <QPushButton>
#include <QStyledItemDelegate>

class SideBarDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SideBarDelegate (QObject *parent = nullptr);
    ~SideBarDelegate () override;

    QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QPushButton *mStyledButton;
};

#endif // SIDEBARDELEGATE_H
