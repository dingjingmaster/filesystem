#ifndef X11WINDOWMANAGER_H
#define X11WINDOWMANAGER_H

#include <QPoint>
#include <QObject>

class X11WindowManager  : public QObject
{
    Q_OBJECT
public:
    static X11WindowManager *getInstance();

    void registerWidget(QWidget *widget);
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit X11WindowManager(QObject *parent = nullptr);

private:
    QPoint mPressPos;
    QPoint mToplevelOffset;
    bool mIsDraging = false;
    QWidget *mCurrentWidget = nullptr;

};

#endif // X11WINDOWMANAGER_H
