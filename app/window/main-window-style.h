#ifndef MAINWINDOWSTYLE_H
#define MAINWINDOWSTYLE_H

#include <QProxyStyle>

class MainWindowStyle : public QProxyStyle
{
    Q_OBJECT
public:
    static MainWindowStyle *getStyle();

private:
    explicit MainWindowStyle(QObject *parent = nullptr);

    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;

};
#endif // MAINWINDOWSTYLE_H
