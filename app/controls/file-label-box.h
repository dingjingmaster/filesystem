#ifndef FILELABELBOX_H
#define FILELABELBOX_H

#include <QListView>
#include <QProxyStyle>

class FileLabelBox : public QListView
{
    Q_OBJECT
public:
    explicit FileLabelBox(QWidget *parent = nullptr);

    QSize sizeHint() const;

    int getTotalDefaultColor() {
        return TOTAL_DEFAULT_COLOR;
    }

    const int TOTAL_DEFAULT_COLOR = 7;

Q_SIGNALS:
    void leftClickOnBlank();

protected:
    void mousePressEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);

};

class LabelBoxStyle : public QProxyStyle
{
    static LabelBoxStyle *getStyle();

    friend class FileLabelBox;
    LabelBoxStyle() {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
};
#endif // FILELABELBOX_H
