#ifndef ICONVIEWSTYLE_H
#define ICONVIEWSTYLE_H

#include <QProxyStyle>

class IconViewStyle : public QProxyStyle
{
    Q_OBJECT
public:
    void release();
    static IconViewStyle *getStyle();
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap) const override;
    void drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled, const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const override;

private:
    explicit IconViewStyle(QStyle *style = nullptr);
    ~IconViewStyle() override {}
};

#endif // ICONVIEWSTYLE_H
