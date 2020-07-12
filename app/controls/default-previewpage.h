#ifndef DEFAULTPREVIEWPAGE_H
#define DEFAULTPREVIEWPAGE_H

#include <memory>
#include <QStackedWidget>
#include <plugin-iface/preview-page-plugin-iface.h>

class QLabel;
class QGridLayout;
class QPushButton;
class QFormLayout;

class FileInfo;
class FileWatcher;
class IconContainer;
class FilePreviewPage;
class FileCountOperation;

class DefaultPreviewPage : public QStackedWidget, public PreviewPageIface
{
    Q_OBJECT
public:
    explicit DefaultPreviewPage(QWidget *parent = nullptr);
    ~DefaultPreviewPage() override;

    void cancel() override;
    void startPreview() override;
    void closePreviewPage() override;
    void prepare(const QString &uri) override;
    void prepare(const QString &uri, PreviewType type) override;

protected:
    void paintEvent(QPaintEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    bool mSupport = true;
    QString mCurrentUri;
    PreviewType mCurrentType;
    QWidget *mEmptyTabWidget;
    std::shared_ptr<FileInfo> mInfo;
    FilePreviewPage *mPreviewTabWidget;
    std::shared_ptr<FileWatcher> mWatcher;
};

class FilePreviewPage : public QFrame
{
    friend class DefaultPreviewPage;
    Q_OBJECT
private:
    explicit FilePreviewPage(QWidget *parent = nullptr);
    ~FilePreviewPage();

private Q_SLOTS:
    void cancel();
    void updateCount();
    void resizeIcon(QSize size);
    void updateInfo(FileInfo *info);
    void countAsync(const QString &uri);

protected Q_SLOTS:
    void resetCount();
    void onPreparedOne(const QString &uri, quint64 size) {
        mFileCount++;
        if (uri.contains("/.")) {
            mHiddenCount++;
        }
        mTotalSize += size;
        this->updateCount();
    }
    void onCountDone();

private:
    quint64 mFileCount = 0;
    quint64 mTotalSize = 0;
    quint64 mHiddenCount = 0;
    FileCountOperation *mCountOp = nullptr;

    QFormLayout *mForm;
    QLabel *mTypeLabel;
    QLabel *mImageSize;
    QGridLayout *mLayout;
    IconContainer *mIcon;
    QLabel *mImageFormat;
    QLabel *mFileCountLabel;
    QLabel *mTotalSizeLabel;
    QLabel *mDisplayNameLabel;
    QLabel *mTimeAccessLabel;
    QLabel *mTimeModifiedLabel;
};

#endif // DEFAULTPREVIEWPAGE_H
