#ifndef CONNECTREMOTEFILESYSTEM_H
#define CONNECTREMOTEFILESYSTEM_H

#include <QDialog>

class ConnectRemoteFilesystem : public QDialog
{
    Q_OBJECT
public:
    explicit ConnectRemoteFilesystem(QWidget *parent = nullptr);
    ~ConnectRemoteFilesystem();

Q_SIGNALS:

public:
    QString uri();
    QString user();
    QString domain();
    QString password();

private:
};

#endif // CONNECTREMOTEFILESYSTEM_H
