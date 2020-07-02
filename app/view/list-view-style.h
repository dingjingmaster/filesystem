#ifndef LISTVIEWSTYLE_H
#define LISTVIEWSTYLE_H

#include <QProxyStyle>

class ListViewStyle : public QProxyStyle
{
    Q_OBJECT
public:
    static ListViewStyle *getStyle();
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;

private:
    explicit ListViewStyle(QObject *parent = nullptr);
};

#endif // LISTVIEWSTYLE_H
