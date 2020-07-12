#ifndef ICONCONTAINER_H
#define ICONCONTAINER_H

#include <QProxyStyle>
#include <QPushButton>



class IconContainer : public QPushButton
{
    Q_OBJECT
public:
    explicit IconContainer(QWidget *parent = nullptr);
    ~IconContainer();

protected:
    void mouseMoveEvent(QMouseEvent *e) {}
    void mousePressEvent(QMouseEvent *e) {}
    void paintEvent(QPaintEvent *e);

private:
    QStyle *m_style;
};

class IconContainerStyle : public QProxyStyle
{
    friend class IconContainer;
    explicit IconContainerStyle();

    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
};

#endif // ICONCONTAINER_H
