#ifndef FILEOPERATION_H
#define FILEOPERATION_H

#include <memory>
#include <QObject>
#include <QRunnable>


class FileOperation : public QObject, public QRunnable
{
    friend class FileOperationManager;

    Q_OBJECT

public:
    enum ResponseType
    {
        Retry,
        Other,
        Cancel,
        Rename,
        Invalid,
        IgnoreOne,
        IgnoreAll,
        BackupOne,
        BackupAll,
        OverWriteOne,
        OverWriteAll,
    };

    explicit FileOperation(QObject *parent = nullptr);
    ~FileOperation();

    virtual void run();

    void setHasError (bool hasError = true);

    bool hasError();

    virtual std::shared_ptr<FileOperationInfo> getOperationInfo();

    void setShouldReversible(bool reversible = true);
    virtual bool reversible();

    bool isCancelled();

Q_SIGNALS:
    void operationStarted ();
    void operationFinished ();
    void operationPrepared ();
    void operationStartSnyc ();
    void operationProgressed ();
    void operationStartRollbacked ();
    void operationAfterProgressed ();
    void operationFallbackRetried ();
    void operationRequestShowWizard ();
    void invalidExited (const QString &message);
    void invalidOperation (const QString &message);
    void operationAfterProgressedOne (const QString &srcUri);
    void operationPreparedOne (const QString &srcUri, const qint64 &size);
    void operationRollbackedOne (const QString &destUri, const QString &srcUri);
    void operationProgressedOne (const QString& srcUri, const QString &destUri, const qint64 &size);
    QVariant errored (const QString &srcUri, const QString &destUri, const GErrorWrapperPtr &err, bool isCritical = false);
    void FileProgressCallback (const QString &srcUri, const QString &destUri, const qint64 &current_file_offset, const qint64 &current_file_size);


public Q_SLOTS:
    virtual void cancel();

protected:
    GCancellableWrapperPtr getCancellable ();

    void notifyFileWatcherOperationFinished ();

private:
    GCancellableWrapperPtr mCancellableWrapper = nullptr;
    bool mIsCancelled = false;
    bool mReversible = false;
    bool mHasError = false;
};

Q_DECLARE_METATYPE(FileOperation::ResponseType)

#endif // FILEOPERATION_H
