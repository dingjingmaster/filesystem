#ifndef SINGLEAPP_H
#define SINGLEAPP_H

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSharedMemory>

class SingleApp : public QCoreApplication
{
    struct InstancesInfo
    {
        bool                primary;
        quint32             secondary;
        qint64              primaryPid;
        quint16             checksum;
    };

    struct ConnectionInfo
    {
        explicit ConnectionInfo() : msgLen(0), instanceId (0), stage (0) {}
        qint64              msgLen;
        quint32             instanceId;
        quint8              stage;
    };

    Q_OBJECT

public:
    enum Mode
    {
        User                    = 1 << 0,
        System                  = 1 << 1,
        SecondaryNotification   = 1 << 2,
        ExcludeAppVersion       = 1 << 3,
        ExcludeAppPath          = 1 << 4
    };

    enum ConnectionType : quint8
    {
        InvalidConnection       = 0,
        NewInstance             = 1,
        SecondaryInstance       = 2,
        Reconnect               = 3
    };

    enum ConnectionStage : quint8
    {
        StageHeader             = 0,
        StageBody               = 1,
        StageConnected          = 2,
    };

    Q_DECLARE_FLAGS(Options, Mode)

    SingleApp (int &argc, char *argv[], const char *appName = "SingleApplication", bool allowSecondary = false, Options options = Mode::User, int timeout = 1000);
    ~SingleApp ();

    bool isPrimary ();
    bool isSecondary ();
    qint64 primaryPid ();
    quint32 instanceId ();
    bool sendMessage (QByteArray message, int timeout = 100);
    void genBlockServerName(const char *appName);

    void startPrimary();
    void startSecondary();
    void connectToPrimary(int msecs, ConnectionType connectionType);

    void readInitMessageHeader(QLocalSocket *socket);
    void readInitMessageBody(QLocalSocket *socket);

private:
    quint16 blockChecksum();
    void initializeMemoryBlock();

public Q_SLOTS:
    void slotConnectionEstablished();
    void slotDataAvailable( QLocalSocket*, quint32 );
    void slotClientConnectionClosed( QLocalSocket*, quint32 );

Q_SIGNALS:
    void instanceStarted ();
    void receivedMessage (quint32 instanceId, QByteArray message);

private:
    QSharedMemory*                          mMemory;
    QLocalSocket                            *mSocket;
    QLocalServer                            *mServer;
    quint32                                 mInstanceNumber;
    QString                                 mBlockServerName;
    Options                                 mOptions;
    QMap<QLocalSocket*, ConnectionInfo>     mConnectionMap;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SingleApp::Options)

#endif // SINGLEAPP_H
