#ifndef GENERICTHUMBNAILER_H
#define GENERICTHUMBNAILER_H

#include <QSize>
#include <QObject>

class GenericThumbnailer : public QObject
{
    Q_OBJECT
public:
    static QIcon generateThumbnail(const QUrl &url, bool shadow = false, const QSize &size = QSize());
    static QIcon generateThumbnail(const QString &path, bool shadow = false, const QSize &size = QSize());
    static QIcon generateThumbnail(const QPixmap &pixmap, bool shadow = true, const QSize &size = QSize());

private:
    explicit GenericThumbnailer(QObject *parent = nullptr);

};

#endif // GENERICTHUMBNAILER_H
