#include "list-view-style.h"

static ListViewStyle *ginstance = nullptr;

ListViewStyle::ListViewStyle(QObject *parent) : QProxyStyle()
{

}

ListViewStyle *ListViewStyle::getStyle()
{
    if (! ginstance) {
        ginstance = new ListViewStyle;
    }

    return  ginstance;
}

void ListViewStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (element == PE_Frame) {
        return;
    }

    if (element == PE_FrameWindow) {
        return;
    }

    return QProxyStyle::drawPrimitive(element, option, painter, widget);
}
