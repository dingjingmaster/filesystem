#ifndef CONNECTREMOTEFILESYSTEM_H
#define CONNECTREMOTEFILESYSTEM_H

#include <QDialog>

class ConnectRemoteFilesystem : public QDialog
{
    Q_OBJECT
public:
    explicit ConnectRemoteFilesystem(QWidget *parent = nullptr);
    ~ConnectRemoteFilesystem();

    QString uri();
    QString user();
    QString domain();
    QString password();

Q_SIGNALS:

public:

private:
};

#endif // CONNECTREMOTEFILESYSTEM_H
