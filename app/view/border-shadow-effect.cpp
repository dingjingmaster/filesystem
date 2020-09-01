#include "border-shadow-effect.h"

#include <QDebug>
#include <QWidget>
#include <QPainter>

//qt's global function
extern void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed);

BorderShadowEffect::BorderShadowEffect(QObject *parent) : QGraphicsEffect(parent)
{

}

void BorderShadowEffect::setBorderRadius(int radius)
{
    mXBorderRadius = radius;
    mYBorderRadius = radius;
}

void BorderShadowEffect::setBorderRadius(int xradius, int yradius)
{
    mXBorderRadius = xradius;
    mYBorderRadius = yradius;
}

void BorderShadowEffect::setBlurRadius(int radius)
{
    mBlurRadius = radius;
}

void BorderShadowEffect::setPadding(int padding)
{
    mPadding = padding;
}

void BorderShadowEffect::setShadowColor(const QColor &color)
{
    if (color != mShadowColor) {
        mForceUpdateCache = true;
    }
    mShadowColor = color;
}

void BorderShadowEffect::setWindowBackground(const QColor &color)
{
    mWindowBg = color;
}

void BorderShadowEffect::setTransParentPath(const QPainterPath &path)
{
    mTransparentPath = path;
}

void BorderShadowEffect::setTransParentAreaBg(const QColor &transparentBg)
{
    mTransparentBg = transparentBg;
}

void BorderShadowEffect::drawWindowShadowManually(QPainter *painter, const QRect &windowRect, bool fakeShadow)
{
    //draw window bg;
    QRect sourceRect = windowRect;
    auto contentRect = sourceRect.adjusted(mPadding, mPadding, -mPadding, -mPadding);
    QPainterPath sourcePath;
    QPainterPath contentPath;
    sourcePath.addRect(sourceRect);
    contentPath.addRoundedRect(contentRect, mXBorderRadius, mYBorderRadius);
    auto targetPath = sourcePath - contentPath;
    auto bgPath = contentPath - mTransparentPath;
    painter->fillPath(bgPath, mWindowBg);
    painter->fillPath(mTransparentPath, mTransparentBg);
    if (fakeShadow) {
        painter->save();
        auto color = mShadowColor;
        color.setAlphaF(0.1);
        painter->setPen(color);
        painter->setBrush(Qt::transparent);
        painter->drawPath(contentPath);
        painter->restore();
        return;
    }

    if (mPadding > 0) {
        if (mCacheShadow.size() == windowRect.size() && !mForceUpdateCache) {
            painter->save();
            painter->setClipPath(sourcePath - contentPath);
            painter->drawImage(QPoint(), mCacheShadow);
            painter->restore();
        } else {
            QPixmap pixmap(sourceRect.size().width(), sourceRect.height());
            pixmap.fill(Qt::transparent);
            QPainter p(&pixmap);
            p.fillPath(contentPath, mShadowColor);
            p.end();
            QImage img = pixmap.toImage();
            qt_blurImage(img, mBlurRadius, false, false);
            pixmap.convertFromImage(img);
            mCacheShadow = img;
            painter->save();
            painter->setClipPath(sourcePath - contentPath);
            painter->drawImage(QPoint(), img);
            painter->restore();
            mForceUpdateCache = false;
        }
    }
}

void BorderShadowEffect::draw(QPainter *painter)
{
    auto sourceRect = boundingRect();
    auto contentRect = boundingRect().adjusted(mPadding, mPadding, -mPadding, -mPadding);
    QPainterPath sourcePath;
    QPainterPath contentPath;
    sourcePath.addRect(sourceRect);
    contentPath.addRoundedRect(contentRect, mXBorderRadius, mYBorderRadius);
    auto targetPath = sourcePath - contentPath;
    painter->fillPath(contentPath, mWindowBg);

    QPoint offset;
    if (sourceIsPixmap()) {
        const QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, QGraphicsEffect::PadToTransparentBorder);
        painter->drawPixmap(offset, pixmap);
    } else {
        const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &offset, QGraphicsEffect::PadToTransparentBorder);
        painter->setWorldTransform(QTransform());
        painter->drawPixmap(offset, pixmap);
    }

    if (mPadding > 0) {
        QPixmap pixmap(sourceRect.size().width(), sourceRect.height());
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.fillPath(contentPath, mShadowColor);
        p.end();
        QImage img = pixmap.toImage();
        qt_blurImage(img, mBlurRadius, false, false);
        pixmap.convertFromImage(img);
        painter->save();
        painter->setClipPath(sourcePath - contentPath);
        painter->drawImage(QPoint(), img);
        painter->restore();
    }
}
