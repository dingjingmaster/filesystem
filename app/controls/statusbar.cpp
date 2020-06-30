#include "status-bar.h"

#include "main/fm-window.h"

#include <QPainter>

StatusBar::StatusBar(FMWindowIface *window, QWidget *parent) : QStatusBar(parent)
{
    mStyledToolbar = new QToolBar;

    setContentsMargins(0, 0, 0, 0);
    setStyleSheet("padding: 0;");
    setSizeGripEnabled(false);
    setMinimumHeight(30);

    mWindow = window;
    mLabel = new QLabel(this);
    mLabel->setContentsMargins(0, 0, 0, 0);
    mLabel->setWordWrap(false);
    mLabel->setAlignment(Qt::AlignCenter);
    addWidget(mLabel, 1);
}

StatusBar::~StatusBar()
{
    mStyledToolbar->deleteLater();
}

void StatusBar::slotUpdate()
{
    if (!mWindow) {
        return;
    }

    auto selections = mWindow->getCurrentSelectionFileInfos();
    if (!selections.isEmpty()) {
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
        g_free(format_size);
    } else {
        mLabel->setText(mWindow->getCurrentUri());
    }
}

void StatusBar::slotUpdate(const QString &message)
{
    mLabel->setText(message);
}

void StatusBar::paintEvent(QPaintEvent *e)
{
    QStatusBar::paintEvent(e);
    QPainter p(this);
    auto rect = this->rect();
    auto bg = mStyledToolbar->palette().window().color();
    p.fillRect(rect, bg);
    auto base = mStyledToolbar->palette().base().color();
    base.setAlpha(0);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(rect, base);
}
