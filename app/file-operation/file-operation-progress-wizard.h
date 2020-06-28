#ifndef FILEOPERATIONPROGRESSWIZARD_H
#define FILEOPERATIONPROGRESSWIZARD_H

#include <QWizard>

class QLabel;
class QProgressBar;
class QFormLayout;
class QGridLayout;
class QSystemTrayIcon;
class FileOperationPreparePage;
class FileOperationProgressPage;
class FileOperationRollbackPage;
class FileOperationAfterProgressPage;

class FileOperationProgressWizard : public QWizard
{
    friend class FileOperationPreparePage;
    friend class FileOperationProgressPage;
    friend class FileOperationAfterProgressPage;
    friend class FileOperationRollbackPage;
    Q_OBJECT
public:
    enum PageId {
        PreparePage,
        ProgressPage
    };

    explicit FileOperationProgressWizard (QWidget *parent = nullptr);
    ~FileOperationProgressWizard () override;

Q_SIGNALS:
    void cancelled ();

public Q_SLOTS:
    virtual void delayShow ();
    virtual void onStartSync ();
    virtual void onElementFoundAll ();
    virtual void switchToRollbackPage ();
    virtual void switchToPreparedPage ();
    virtual void switchToProgressPage ();
    virtual void switchToAfterProgressPage ();
    virtual void onFileOperationProgressedAll ();
    virtual void onElementClearOne (const QString &uri);
    virtual void onElementFoundOne (const QString &uri, const qint64 &size);
    virtual void onFileRollbacked (const QString &destUri, const QString &srcUri);
    virtual void onFileOperationProgressedOne (const QString &uri, const QString &destUri, const qint64 &size);
    virtual void updateProgress (const QString &srcUri, const QString &destUri, quint64 current, quint64 total);

protected:
    void closeEvent(QCloseEvent *e) override;

    int mTotalCount = 0;
    int mCurrentSize = 0;
    qint64 mTotalSize = 0;
    int mCurrentCount = 1;

    FileOperationRollbackPage *mLastPage = nullptr;
    FileOperationPreparePage *mFirstPage = nullptr;
    FileOperationProgressPage *mSecondPage = nullptr;
    FileOperationAfterProgressPage *mThirdPage = nullptr;

private:
    QSystemTrayIcon *mTrayIcon = nullptr;
    QTimer *mDelayer;
};

class FileOperationPreparePage : public QWizardPage
{
    friend class FileOperationProgressWizard;
    Q_OBJECT
public:
    explicit FileOperationPreparePage (QWidget *parent = nullptr);
    ~FileOperationPreparePage ();

Q_SIGNALS:
    void cancelled ();

private:
    QLabel *mSrcLine = nullptr;
    QLabel *mStateLine = nullptr;
    QFormLayout *mLayout = nullptr;
};

class FileOperationProgressPage : public QWizardPage
{
    friend class FileOperationProgressWizard;
    Q_OBJECT
public:
    explicit FileOperationProgressPage (QWidget *parent = nullptr);
    ~FileOperationProgressPage ();

Q_SIGNALS:
    void cancelled ();

private:
    QLabel *mSrcLine = nullptr;
    QLabel *mDestLine = nullptr;
    QLabel *mStateLine = nullptr;
    QGridLayout *mLayout = nullptr;
    QProgressBar *mProgressBar = nullptr;
};

class FileOperationAfterProgressPage : public QWizardPage
{
    friend class FileOperationProgressWizard;
    Q_OBJECT
public:
    explicit FileOperationAfterProgressPage (QWidget *parent = nullptr);
    ~FileOperationAfterProgressPage ();

private:
    int mFileDeletedCount = 0;
    QLabel *mSrcLine = nullptr;
    QGridLayout *mLayout = nullptr;
    QProgressBar *mProgressBar = nullptr;
};

class FileOperationRollbackPage : public QWizardPage
{
    Q_OBJECT
    friend class FileOperationProgressWizard;
public:
    explicit FileOperationRollbackPage (QWidget *parent = nullptr);
    ~FileOperationRollbackPage ();

private:
    int mCurrentCount = 0;
    QGridLayout *mLayout = nullptr;
    QProgressBar *mProgressBar = nullptr;
};

#endif // FILEOPERATIONPROGRESSWIZARD_H
