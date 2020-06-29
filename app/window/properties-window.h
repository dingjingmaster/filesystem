#ifndef PROPERTIESWINDOW_H
#define PROPERTIESWINDOW_H

#include <QObject>

class PropertiesWindow : public QObject
{
    Q_OBJECT
public:
    explicit PropertiesWindow(QObject *parent = nullptr);

Q_SIGNALS:

};

#endif // PROPERTIESWINDOW_H
