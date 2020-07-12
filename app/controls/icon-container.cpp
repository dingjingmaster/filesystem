#include "icon-container.h"

#include <QPainter>
#include <qstyleoption.h>

IconContainer::IconContainer(QWidget *parent) : QPushButton(parent)
{
    setEnabled(true);
    setCheckable(false);
    setDefault(true);
    setFlat(true);

    m_style = new IconContainerStyle;
    setStyle(m_style);
}

IconContainer::~IconContainer()
{
    m_style->deleteLater();
}

void IconContainer::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.fillRect(this->rect(), this->palette().base());
    p.setPen(this->palette().dark().color());
    //p.drawRect(this->rect().adjusted(0, 0, -1, -1));
    QPushButton::paintEvent(e);
}

IconContainerStyle::IconContainerStyle() : QProxyStyle()
{

}

void IconContainerStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    QStyleOptionButton opt = *qstyleoption_cast<const QStyleOptionButton *>(option);
    opt.palette.setColor(QPalette::Highlight, Qt::transparent);
    return QProxyStyle::drawControl(element, &opt, painter, widget);
}
