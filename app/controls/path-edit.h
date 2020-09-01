#ifndef PATHEDIT_H
#define PATHEDIT_H

#include <QLineEdit>

class PathBarModel;
class PathComplete;

class PathEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit PathEdit(QWidget *parent = nullptr);

Q_SIGNALS:
    void editCancelled();
    void uriChangeRequest(const QString &uri);

public Q_SLOTS:
    void setUri(const QString &uri);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    QString mLastUri;
    PathBarModel *mModel;
    PathComplete *mCompleter;
    bool mRightClick = false;
};

#endif // PATHEDIT_H
