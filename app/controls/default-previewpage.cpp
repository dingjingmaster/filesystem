#include "default-previewpage.h"
#include "icon-container.h"

#include <QEvent>
#include <QLabel>
#include <memory>
#include <QResizeEvent>
#include <QPainter>
#include <qgridlayout.h>
#include <QFormLayout>
#include <thumbnail-manager.h>
#include <QTime>
#include <QLocale>
#include <QIcon>
#include <QThreadPool>
#include <QImageReader>

#include <file/file-count-operation.h>
#include <file/file-info-job.h>
#include <file/file-info-manager.h>
#include <file/file-watcher.h>

DefaultPreviewPage::DefaultPreviewPage(QWidget *parent) : QStackedWidget (parent)
{
    auto label = new QLabel(tr("Select the file you want to preview..."), this);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    mEmptyTabWidget = label;

    /*
    label = new QLabel(this);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    */
    auto previewPage = new FilePreviewPage(this);
    previewPage->installEventFilter(this);
    mPreviewTabWidget = previewPage;

    addWidget(mPreviewTabWidget);
    addWidget(mEmptyTabWidget);

    setCurrentWidget(mEmptyTabWidget);
}

DefaultPreviewPage::~DefaultPreviewPage()
{
    if (mInfo && mInfo.use_count() <= 2) {
        FileInfoManager::getInstance()->remove(mInfo);
    }
}

bool DefaultPreviewPage::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == mPreviewTabWidget) {
        if (ev->type() == QEvent::Resize) {
            auto e = static_cast<QResizeEvent*>(ev);
            auto page = qobject_cast<FilePreviewPage*>(mPreviewTabWidget);
            int width = e->size().width()/3;
            width = width>96? width: 96;
            page->resizeIcon(QSize(width, width));
        }
    }
    return QStackedWidget::eventFilter(obj, ev);
}

void DefaultPreviewPage::prepare(const QString &uri, PreviewType type)
{
    mCurrentUri = uri;
    mInfo = FileInfo::fromUri(uri);
    mCurrentType = type;
    mSupport = uri.contains("file:///");
    mWatcher = std::make_shared<FileWatcher>(uri);
    connect(mWatcher.get(), &FileWatcher::locationChanged, [=](const QString &, const QString &newUri) {
        this->prepare(newUri);
        this->startPreview();
    });
    mWatcher->startMonitor();
}

void DefaultPreviewPage::prepare(const QString &uri)
{
    prepare(uri, Other);
}

void DefaultPreviewPage::startPreview()
{
    if (mSupport) {
        auto previewPage = qobject_cast<FilePreviewPage*>(mPreviewTabWidget);
        auto info = FileInfo::fromUri(mCurrentUri);
        previewPage->updateInfo(mInfo.get());
        setCurrentWidget(previewPage);
    } else {
        QLabel *label = qobject_cast<QLabel*>(mEmptyTabWidget);
        label->setText(tr("Can not preview this file."));
    }
}

void DefaultPreviewPage::cancel()
{
    mPreviewTabWidget->cancel();
    setCurrentWidget(mEmptyTabWidget);
    QLabel *label = qobject_cast<QLabel*>(mEmptyTabWidget);
    label->setText(tr("Select the file you want to preview..."));
}

void DefaultPreviewPage::closePreviewPage()
{
    cancel();
    deleteLater();
}

void DefaultPreviewPage::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(this->rect(), this->palette().base());
    QStackedWidget::paintEvent(e);
}

FilePreviewPage::FilePreviewPage(QWidget *parent) : QFrame(parent)
{
    mLayout = new QGridLayout(this);
    setLayout(mLayout);

    mIcon = new IconContainer(this);
    mIcon->setIconSize(QSize(96, 96));
    mLayout->addWidget(mIcon);

    mForm = new QFormLayout(this);
    mDisplayNameLabel = new QLabel(this);
    mDisplayNameLabel->setWordWrap(true);
    mForm->addRow(tr("File Name:"), mDisplayNameLabel);
    mTypeLabel = new QLabel(this);
    mForm->addRow(tr("File Type:"), mTypeLabel);
    mTimeAccessLabel = new QLabel(this);
    mForm->addRow(tr("Time Access:"), mTimeAccessLabel);
    mTimeModifiedLabel = new QLabel(this);
    mForm->addRow(tr("Time Modified:"), mTimeModifiedLabel);
    mFileCountLabel = new QLabel(this);
    mForm->addRow(tr("Children Count:"), mFileCountLabel);
    mTotalSizeLabel = new QLabel(this);
    mForm->addRow(tr("Size:"), mTotalSizeLabel);

    //image
    mImageSize = new QLabel(this);
    mForm->addRow(tr("Image size:"), mImageSize);
    mImageFormat = new QLabel(this);
    mForm->addRow(tr("Image format:"), mImageFormat);

    mForm->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    mForm->setFormAlignment(Qt::AlignHCenter);
    mForm->setLabelAlignment(Qt::AlignRight);

    QWidget *form = new QWidget(this);
    form->setLayout(mForm);
    mLayout->addWidget(form, 1, 0);
}

FilePreviewPage::~FilePreviewPage()
{

}

void FilePreviewPage::updateInfo(FileInfo *info)
{
    if (info->displayName().isEmpty()) {
        FileInfoJob j(info->uri());
        j.querySync();
    }
    auto thumbnail = ThumbnailManager::getInstance()->tryGetThumbnail(info->uri());
    if (!thumbnail.isNull()) {
        QUrl url = info->uri();
        thumbnail.addFile(url.path());
    }
    auto icon = QIcon::fromTheme(info->iconName(), QIcon::fromTheme("text-x-generic"));
    mIcon->setIcon(thumbnail.isNull()? icon: thumbnail);
    //mIcon->setIcon(info->thumbnail().isNull()? QIcon::fromTheme(info->iconName()): info->thumbnail());
    mDisplayNameLabel->setText(info->displayName());
    mTypeLabel->setText(info->fileType());

    QLocale locale;
    auto access = QDateTime::fromMSecsSinceEpoch(info->accessTime()*1000);
    auto modify = QDateTime::fromMSecsSinceEpoch(info->modifiedTime()*1000);
    if (locale.language() == QLocale::Chinese)
    {
        mTimeAccessLabel->setText(access.toString(Qt::SystemLocaleShortDate));
        mTimeModifiedLabel->setText(modify.toString(Qt::SystemLocaleShortDate));
    }
    else
    {
        mTimeAccessLabel->setText(access.toString(Qt::ISODate));
        mTimeModifiedLabel->setText(modify.toString(Qt::ISODate));
    }

    mFileCountLabel->setText(tr(""));
    if (info->isDir()) {
        mForm->itemAt(4, QFormLayout::LabelRole)->widget()->setVisible(true);
        mFileCountLabel->setVisible(true);
    } else {
        mForm->itemAt(4, QFormLayout::LabelRole)->widget()->setVisible(false);
        mFileCountLabel->setVisible(false);
    }

    if (info->mimeType().startsWith("image/")) {
        QUrl url = info->uri();
        QImageReader r(url.path());
        auto image_size_row_left = mForm->itemAt(6, QFormLayout::LabelRole)->widget();
        image_size_row_left->setVisible(true);

        mImageSize->setText(tr("%1x%2").arg(r.size().width()).arg(r.size().height()));
        auto thumbnail = ThumbnailManager::getInstance()->tryGetThumbnail(info->uri());
        bool rgba = thumbnail.pixmap(r.size()).hasAlphaChannel();
        mImageSize->setVisible(true);
        auto image_format_row_left = mForm->itemAt(7, QFormLayout::LabelRole)->widget();
        image_format_row_left->setVisible(true);
        mImageFormat->setText(rgba? "RGBA": "RGB");
        mImageFormat->setVisible(true);
    } else {
        auto image_size_row_left = mForm->itemAt(6, QFormLayout::LabelRole)->widget();
        image_size_row_left->setVisible(false);
        mImageSize->setVisible(false);
        auto image_format_row_left = mForm->itemAt(7, QFormLayout::LabelRole)->widget();
        image_format_row_left->setVisible(false);
        mImageFormat->setVisible(false);
    }
    if (info->fileType().startsWith("video/")) {

    }
    if (info->fileType().startsWith("audio/")) {

    }

    countAsync(info->uri());
}

void FilePreviewPage::countAsync(const QString &uri)
{
    cancel();
    mFileCount = 0;
    mTotalSize = 0;
    mHiddenCount = 0;

    QStringList uris;
    uris<<uri;
    auto info = FileInfo::fromUri(uri);
    mCountOp = new FileCountOperation(uris, !info->isDir());
    connect(mCountOp, &FileOperation::operationStarted, this, &FilePreviewPage::resetCount, Qt::BlockingQueuedConnection);
    connect(mCountOp, &FileOperation::operationPreparedOne, this, &FilePreviewPage::onPreparedOne, Qt::BlockingQueuedConnection);
    connect(mCountOp, &FileCountOperation::countDone, this, &FilePreviewPage::onCountDone, Qt::BlockingQueuedConnection);
    QThreadPool::globalInstance()->start(mCountOp);
}

void FilePreviewPage::updateCount()
{
    mFileCountLabel->setText(tr("%1 total, %2 hidden").arg(mFileCount).arg(mHiddenCount));
    auto format = g_format_size(mTotalSize);
    mTotalSizeLabel->setText(format);
    g_free(format);
}

void FilePreviewPage::cancel()
{
    if (mCountOp) {
        mCountOp->blockSignals(true);
        mCountOp->slotCancel();
        onCountDone();
    }
    mCountOp = nullptr;
}

void FilePreviewPage::resizeIcon(QSize size)
{
    mIcon->setIconSize(size);
}

void FilePreviewPage::resetCount()
{
    mFileCount = 0;
    mTotalSize = 0;
    mHiddenCount = 0;
    updateCount();
}

void FilePreviewPage::onCountDone()
{
    if (!mCountOp) {
        return;
    }

    mCountOp->getInfo(mFileCount, mHiddenCount, mTotalSize);
    this->updateCount();
    mCountOp = nullptr;
    mFileCount = 0;
    mHiddenCount = 0;
    mTotalSize = 0;
}
