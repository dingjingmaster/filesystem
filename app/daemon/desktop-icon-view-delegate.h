#ifndef DESKTOPICONVIEWDELEGATE_H
#define DESKTOPICONVIEWDELEGATE_H

#include <QPushButton>
#include <QStyledItemDelegate>

class DesktopIconView;

class DesktopIconViewDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DesktopIconViewDelegate(QObject *parent = nullptr);
    ~DesktopIconViewDelegate() override;

    DesktopIconView *getView() const;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    /**
     * @brief 绘制桌面背景、图标
     * @param painter
     * @param option
     * @param index
     */
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;


    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QPushButton *mStyledButton;
};

#endif // DESKTOPICONVIEWDELEGATE_H
