#include "main-window-style.h"

static MainWindowStyle *global_instance = nullptr;

MainWindowStyle *MainWindowStyle::getStyle()
{
    if (!global_instance) {
        global_instance = new MainWindowStyle;
    }
    return global_instance;
}

MainWindowStyle::MainWindowStyle(QObject *parent) : QProxyStyle()
{

}

int MainWindowStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch (metric) {
    case PM_LayoutVerticalSpacing:
    case PM_LayoutTopMargin:
    case PM_LayoutLeftMargin:
    case PM_LayoutRightMargin:
    case PM_LayoutBottomMargin:
    case PM_LayoutHorizontalSpacing:
    case PM_DockWidgetFrameWidth:
    case PM_DockWidgetTitleMargin:
    case PM_DockWidgetTitleBarButtonMargin:
        return 0;
    case PM_DockWidgetHandleExtent:
        return 2;
    case PM_DockWidgetSeparatorExtent:
        return 2;
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}
