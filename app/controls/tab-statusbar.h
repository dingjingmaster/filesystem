#ifndef TABSTATUSBAR_H
#define TABSTATUSBAR_H

#include <QStatusBar>

class QLabel;
class QToolBar;
class TabWidget;
class ElidedLabel;
class QSlider;

class TabStatusBar : public QStatusBar
{
    friend class MainWindow;
    friend class TabWidget;
    Q_OBJECT
public:
    explicit TabStatusBar(TabWidget *tab, QWidget *parent = nullptr);
    ~TabStatusBar() override;

    int currentZoomLevel();

Q_SIGNALS:
    void zoomLevelChangedRequest(int zoomLevel);

public Q_SLOTS:
    void update();
    void onZoomRequest(bool zoomIn);
    void update(const QString &message);
    void updateZoomLevelState(int zoomLevel);

protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    QSlider *mSlider;
    TabWidget *mTab = nullptr;
    ElidedLabel *mLabel = nullptr;
    QToolBar *mStyledToolbar = nullptr;
};

class ElidedLabel : public QWidget
{
    Q_OBJECT
public:
    explicit ElidedLabel(QWidget *parent);

    void setText(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString mText;
};


#endif // TABSTATUSBAR_H
