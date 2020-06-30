#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QLabel>
#include <QStatusBar>
#include <QToolBar>

class FMWindowIface;

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit StatusBar (FMWindowIface *window, QWidget *parent = nullptr);
    ~StatusBar () override;

public Q_SLOTS:
    void slotUpdate ();
    void slotUpdate (const QString &message);

protected:
    void paintEvent (QPaintEvent *e) override;

private:
    QLabel *mLabel = nullptr;
    FMWindowIface *mWindow = nullptr;
    QToolBar *mStyledToolbar = nullptr;
};

#endif // STATUSBAR_H
