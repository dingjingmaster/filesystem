#include "tab-statusbar.h"

#include <QSlider>
#include <QToolBar>
#include <QPainter>
#include <QApplication>
#include <file-utils.h>
#include <global-settings.h>
#include <window/main-window.h>
#include <vfs/search-vfs-uri-parser.h>

TabStatusBar::TabStatusBar(TabWidget *tab, QWidget *parent) : QStatusBar(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    mStyledToolbar = new QToolBar;

    setContentsMargins(0, 0, 0, 0);
    setStyleSheet("padding: 0;");
    setSizeGripEnabled(false);
    setMinimumHeight(30);

    mTab = tab;
    mLabel = new ElidedLabel(this);
    mLabel->setContentsMargins(25, 0, 0, 0);
    addWidget(mLabel, 1);

    mSlider = new QSlider(Qt::Horizontal, this);
    mSlider->setRange(0, 100);

    auto mainWindow = qobject_cast<MainWindow *>(this->topLevelWidget());
    auto settings = GlobalSettings::getInstance();
    int defaultZoomLevel = settings->getValue(DEFAULT_VIEW_ID).toInt();
    if (mainWindow) {
        defaultZoomLevel = mainWindow->currentViewZoomLevel();
    }
    mSlider->setValue(defaultZoomLevel);

    connect(mSlider, &QSlider::valueChanged, this, &TabStatusBar::zoomLevelChangedRequest);
}

TabStatusBar::~TabStatusBar()
{
    mStyledToolbar->deleteLater();
}

int TabStatusBar::currentZoomLevel()
{
    if (mSlider->isEnabled()) {
        mSlider->value();
    }
    return -1;
}

void TabStatusBar::update()
{
    if (!mTab)
        return;

    auto selections = mTab->getCurrentSelectionFileInfos();
    if (! selections.isEmpty()) {
        QString directoriesString;
        int directoryCount = 0;
        QString filesString;
        int fileCount = 0;
        goffset size = 0;
        for (auto selection : selections) {
            if(selection->isDir()) {
                directoryCount++;
            } else if (!selection->isVolume()) {
                fileCount++;
                size += selection->size();
            }
        }
        auto format_size = g_format_size(size);
        if (selections.count() == 1) {
            if (directoryCount == 1)
                directoriesString = QString(", %1").arg(selections.first()->displayName());
            if (fileCount == 1)
                filesString = QString(", %1, %2").arg(selections.first()->displayName()).arg(format_size);
        } else if (directoryCount > 1 && (fileCount > 1)) {
            directoriesString = tr("; %1 folders").arg(directoryCount);
            filesString = tr("; %1 files, %2 total").arg(fileCount).arg(format_size);
        } else if (directoryCount > 1 && (fileCount > 1)) {
            directoriesString = tr("; %1 folder").arg(directoryCount);
            filesString = tr("; %1 file, %2").arg(fileCount).arg(format_size);
        } else if (fileCount == 0) {
            directoriesString = tr("; %1 folders").arg(directoryCount);
        } else {
            filesString = tr("; %1 files, %2 total").arg(fileCount).arg(format_size);
        }

        mLabel->setText(tr("%1 selected").arg(selections.count()) + directoriesString + filesString);
        //showMessage(tr("%1 files selected ").arg(selections.count()));
        g_free(format_size);
    }
    else {
        auto uri = mTab->getCurrentUri();
        auto displayName = FileUtils::getFileDisplayName(uri);
        if (uri.startsWith("search:///"))
        {
            QString nameRegexp = SearchVFSUriParser::getSearchUriNameRegexp(uri);
            QString targetDirectory = SearchVFSUriParser::getSearchUriTargetDirectory(uri);
            displayName = tr("Search \"%1\" in \"%2\"").arg(nameRegexp).arg(targetDirectory);
            mLabel->setText(displayName);
        } else {
            mLabel->setText(mTab->getCurrentUri());
        }
    }
}

void TabStatusBar::update(const QString &message)
{
    mLabel->setText(message);
}

void TabStatusBar::updateZoomLevelState(int zoomLevel)
{
    mSlider->setValue(zoomLevel);
}

void TabStatusBar::onZoomRequest(bool zoomIn)
{
    int value = mSlider->value();
    if (zoomIn) {
        ++ value;
    } else {
        -- value;
    }
    mSlider->setValue(value);
}


void TabStatusBar::paintEvent(QPaintEvent *e)
{
    return;
}

void TabStatusBar::mousePressEvent(QMouseEvent *e)
{
    return;
}

void TabStatusBar::resizeEvent(QResizeEvent *e)
{
    QStatusBar::resizeEvent(e);
    auto pos = this->rect().topRight();
    auto size = mSlider->size();
    mSlider->move(pos.x() - size.width() - 20, this->size().height()/2 - size.height()/2);
}

ElidedLabel::ElidedLabel(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setContentsMargins(30, 0, 120, 0);
}

void ElidedLabel::setText(const QString &text)
{
    mText = text;
    this->update();
}

void ElidedLabel::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    bool active = opt.state &QStyle::State_Active;

    QColor base = active? qApp->palette().color(QPalette::Active, QPalette::Base): qApp->palette().color(QPalette::Inactive, QPalette::Base);

    QFontMetrics fm(this->font());
    auto elidedText = fm.elidedText(mText, Qt::TextElideMode::ElideRight, this->size().width() - 150);
    QPainter p(this);
    //p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    QLinearGradient linearGradient;
    linearGradient.setStart(QPoint(10, this->height()));
    linearGradient.setFinalStop(QPoint(10, 0));
    linearGradient.setColorAt(0, base);
    linearGradient.setColorAt(0.75, base);
    linearGradient.setColorAt(1, Qt::transparent);

    int overlap = qApp->style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarOverlap);
    int layoutWidth = qApp->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    int adjustedY2 = qMin(0, overlap - layoutWidth);

    QPainterPath path;
    path.addRect(this->rect().adjusted(0, 0, adjustedY2 - this->height(), 0));

    p.fillPath(path, linearGradient);

    QPainterPath path2;

    int radius = this->height();
    QPoint pos = QPoint(this->width() + adjustedY2 - this->height(), this->height());
    QRect targetRect = QRect(pos.x() - radius, pos.y() - radius, radius*2, radius*2);
    path2.moveTo(pos);
    path2.arcTo(targetRect, 0, 90);

    QRadialGradient radialGradient;
    radialGradient.setCenter(pos);
    radialGradient.setFocalPoint(pos);
    radialGradient.setRadius(radius);
    radialGradient.setColorAt(0, base);
    radialGradient.setColorAt(0.75, base);
    radialGradient.setColorAt(1, Qt::transparent);
    p.fillPath(path2, radialGradient);

    style()->drawItemText(&p, this->rect().adjusted(30, 0, -120, 0), Qt::AlignLeft, qApp->palette(), this->isEnabled(), elidedText, QPalette::WindowText);
}

