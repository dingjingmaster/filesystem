#ifndef PDFTHUMBNAIL_H
#define PDFTHUMBNAIL_H

#include <QPixmap>
#include <QString>



class PdfThumbnail
{
public:
    explicit PdfThumbnail(const QString &url, unsigned int pageNum = 0);
    ~PdfThumbnail();

    QPixmap generateThumbnail(unsigned int pageNum = 0);

public:
    unsigned int pageNum;
private:
    QString shortUrl;
//    Poppler::Page *pagePrivate = nullptr;
//    Poppler::Document *documentPrivate = nullptr;
};

#endif // PDFTHUMBNAIL_H
