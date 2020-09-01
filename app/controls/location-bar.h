#ifndef LOCATIONBAR_H
#define LOCATIONBAR_H

#include <QToolBar>

class QLineEdit;

class LocationBar : public QToolBar
{
    Q_OBJECT
public:
    explicit LocationBar(QWidget *parent = nullptr);
    ~LocationBar() override;
    const QString getCurentUri() {
        return mCurrentUri;
    }

Q_SIGNALS:
    void blankClicked();
    void groupChangedRequest(const QString &uri);

public Q_SLOTS:
    void setRootUri(const QString &uri);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void addButton(const QString &uri, bool setIcon = false, bool setMenu = true);

private:
    QString mCurrentUri;
    QLineEdit *mStyledEdit;
};

#endif // LOCATIONBAR_H
