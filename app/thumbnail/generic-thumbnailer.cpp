#include "generic-thumbnailer.h"

#include <QUrl>
#include <QIcon>
#include <QFile>
#include <QPainter>
#include <QFileInfo>
#include <QFileInfo>


extern void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed);

QIcon GenericThumbnailer::generateThumbnail(const QUrl &url, bool shadow, const QSize &size)
{
    QIcon icon;
    QFile file(url.path());
    if (!file.exists()) {
        return icon;
    }

    if (url.path().endsWith(".svg")) {
        if (file.size() < 1024 * 1024 * 8)
            icon.addFile(url.path());
        return icon;
    }

    QImage img(url.path());

    if (img.rect().size().width() > 128) {
        if (size.isValid()) {
            img = img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        } else {
            img = img.scaledToWidth(128, Qt::SmoothTransformation);
        }
    }

    if (img.hasAlphaChannel()) {
        icon.addPixmap(QPixmap::fromImage(img));
        return icon;
    }

    if (shadow) {
        QPixmap pixmap = QPixmap::fromImage(img);
        pixmap = pixmap.scaled(img.rect().adjusted(4, 4, -4, -4).size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QImage newImg(img.size(), QImage::Format_ARGB32);
        newImg.fill(Qt::transparent);
        QPainter p(&newImg);

        p.setPen(Qt::transparent);
        p.setBrush(Qt::gray);
        p.drawRect(newImg.rect().adjusted(4, 4, -4, -4));

        qt_blurImage(newImg, 4, false, false);
        p.drawPixmap(newImg.rect().adjusted(4, 4, -4, -4), pixmap);

        p.end();
        icon.addPixmap(QPixmap::fromImage(newImg));
    } else {
        icon.addPixmap(QPixmap::fromImage(img));
    }

    return icon;
}

QIcon GenericThumbnailer::generateThumbnail(const QString &path, bool shadow, const QSize &size)
{
    QIcon icon;
    QFile file(path);

    if (!file.exists()) {
        return icon;
    }

    if (path.endsWith(".svg")) {
        if (file.size() < 1024 * 1024 * 8)
            icon.addFile(path);
        return icon;
    }

    QImage img(path);
    if (img.rect().size().width() > 128) {
        if (size.isValid()) {
            img = img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        } else {
            img = img.scaledToWidth(128, Qt::SmoothTransformation);
        }
    }

    if (img.hasAlphaChannel()) {
        icon.addPixmap(QPixmap::fromImage(img));
        return icon;
    }

    if (shadow) {
        QPixmap pixmap = QPixmap::fromImage(img);
        pixmap = pixmap.scaled(img.rect().adjusted(4, 4, -4, -4).size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QImage newImg(img.size(), QImage::Format_ARGB32);
        newImg.fill(Qt::transparent);
        QPainter p(&newImg);

        p.setPen(Qt::transparent);
        p.setBrush(Qt::gray);
        p.drawRect(newImg.rect().adjusted(4, 4, -4, -4));

        qt_blurImage(newImg, 4, false, false);
        p.drawPixmap(newImg.rect().adjusted(4, 4, -4, -4), pixmap);

        p.end();
        icon.addPixmap(QPixmap::fromImage(newImg));
    } else {
        icon.addPixmap(QPixmap::fromImage(img));
    }

    return icon;
}

QIcon GenericThumbnailer::generateThumbnail(const QPixmap &pixmap, bool shadow, const QSize &size)
{
    QIcon icon;
    QPixmap tmp = pixmap;
    if (pixmap.isNull())
        return icon;

    QSize realSize;
    if (size.isValid()) {
        realSize = size;
        tmp = tmp.scaled(size);
    } else {
        tmp = tmp.scaledToWidth(128, Qt::SmoothTransformation);
        realSize = tmp.size();
    }

    if (shadow) {
        QImage newImg(realSize, QImage::Format::Format_ARGB32);
        newImg.fill(Qt::transparent);

        tmp = tmp.scaled(tmp.rect().adjusted(4, 4, -4, -4).size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QPainter p(&newImg);

        p.setPen(Qt::transparent);
        p.setBrush(Qt::gray);
        p.drawRect(newImg.rect().adjusted(4, 4, -4, -4));

        qt_blurImage(newImg, 4, false, false);
        p.drawPixmap(newImg.rect().adjusted(4, 4, -4, -4), tmp);

        p.end();
        icon.addPixmap(QPixmap::fromImage(newImg));
    } else {
        icon.addPixmap(tmp);
    }
    return icon;
}

GenericThumbnailer::GenericThumbnailer(QObject *parent) : QObject(parent)
{

}
