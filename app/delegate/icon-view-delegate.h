#ifndef ICONVIEWDELEGATE_H
#define ICONVIEWDELEGATE_H

#include <QStyledItemDelegate>

class IconView;
class QPushButton;
class IconViewTextHelper;
class DesktopIndexWidget;
class IconViewIndexWidget;
class DesktopIconViewDelegate;

class IconViewDelegate : public QStyledItemDelegate
{
    friend class IconViewIndexWidget;
    Q_OBJECT
public:
    explicit IconViewDelegate(QObject *parent = nullptr);
    ~IconViewDelegate() override;

    IconView *getView() const;
    const QBrush selectedBrush() const;
    void setMaxLineCount(int count = 0);

public Q_SLOTS:
    void setCutFiles(const QModelIndexList &indexes);

protected:
    void setIndexWidget(const QModelIndex &index, QWidget *widget) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QWidget *mIndexWidget;
    QPushButton *mStyledButton;
    QModelIndexList mCutIndexes;
    QModelIndex mIndexWidgetIndex;
};

class IconViewTextHelper
{
    friend class IconViewDelegate;
    friend class IconViewIndexWidget;
    friend class DesktopIndexWidget;
    friend class DesktopIconViewDelegate;

    static QSize getTextSizeForIndex(const QStyleOptionViewItem &option, const QModelIndex &index, int horizalMargin = 0, int maxLineCount = 0);
    static void paintText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index, int textMaxHeight, int horizalMargin = 0, int maxLineCount = 0, bool useSystemPalette = true);
};


#endif // ICONVIEWDELEGATE_H
