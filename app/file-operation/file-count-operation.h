#ifndef FILECOUNTOPERATION_H
#define FILECOUNTOPERATION_H

#include "file-operation.h"

class FileNodeReporter;

class FileCountOperation : public FileOperation
{
    Q_OBJECT
public:
    explicit FileCountOperation (const QStringList &uris, bool countRoot = true, QObject *parent = nullptr);
    ~FileCountOperation () override;

    void run () override;
    std::shared_ptr<FileOperationInfo> getOperationInfo () override;
    void getInfo (quint64 &file_count, quint64 &hidden_file_count, quint64 &total_size);

Q_SIGNALS:
    void countDone (quint64 file_count, quint64 hidden_file_count, quint64 total_size);

public Q_SLOTS:
    void slotCancel() override;

private:
    QStringList mUris;

    bool mCountRoot = true;
    quint64 mFileCount = 0;
    quint64 mTotalSize = 0;
    quint64 mHiddenFileCount = 0;
    FileNodeReporter *mReporter = nullptr;

};

#endif // FILECOUNTOPERATION_H
