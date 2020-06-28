#ifndef FILENODEREPORTER_H
#define FILENODEREPORTER_H

#include "file-operation.h"

class FileNode;

class FileNodeReporter : public FileOperation
{
    Q_OBJECT
public:
    explicit FileNodeReporter (QObject *parent = nullptr);
    ~FileNodeReporter ();

    void slotCancel ();
    bool isOperationCancelled ();
    void sendNodeFound (const QString &uri, const qint64 &offset);

Q_SIGNALS:
    void enumerateNodeFinished ();
    void nodeFound (const QString &uri, const qint64 &offset);
    void nodeOperationDone (const QString &uri, const qint64 &offset);

private:
    bool mCancelled = false;
};

#endif // FILENODEREPORTER_H
