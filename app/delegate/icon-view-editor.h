#ifndef ICONVIEWEDITOR_H
#define ICONVIEWEDITOR_H

#include <QTextEdit>


class QLabel;
class QLineEdit;


class IconViewEditor : public QTextEdit
{
    Q_OBJECT
public:
    explicit IconViewEditor(QWidget *parent = nullptr);
    ~IconViewEditor() override;

Q_SIGNALS:
    void returnPressed();

public Q_SLOTS:
    void minimalAdjust();

protected:
    void paintEvent(QPaintEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    QLineEdit *mStyledEdit;
};
#endif // ICONVIEWEDITOR_H
