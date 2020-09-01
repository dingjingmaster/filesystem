#ifndef BORDERSHADOWEFFECT_H
#define BORDERSHADOWEFFECT_H

#include <QGraphicsEffect>

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#include <QPainterPath>
#endif

/*!
 * \brief The BorderShadowEffect class
 * \details
 * This class is used to decorate a frameless window.
 * It provides a border shadow which can be adjusted.
 *
 * The effect is similar to QGraphicsDropShadowEffects,
 * but it doesn't blur allow the window. It just render
 * a border for toplevel window, whatever if the window
 * is transparent and whatever element on the window.
 *
 * \note
 * To let the effect works,
 * You have to use it on toplevel window, and let the
 * window has an invisible contents margins.
 *
 * If your window has a border radius, use setBorderRadius()
 * for matching your window border rendering.
 */
class BorderShadowEffect : public QGraphicsEffect
{
    Q_OBJECT
public:
    explicit BorderShadowEffect(QObject *parent = nullptr);
    void setPadding(int padding);
    void setBlurRadius(int radius);
    void setBorderRadius(int radius);
    void setShadowColor(const QColor &color);
    void setWindowBackground(const QColor &color);
    void setBorderRadius(int xradius, int yradius);
    void setTransParentPath(const QPainterPath &path);
    void setTransParentAreaBg(const QColor &transparentBg);
    void drawWindowShadowManually(QPainter *painter, const QRect &windowRect, bool fakeShadow = false);

protected:
    void draw(QPainter *painter) override;

private:
    int mXBorderRadius = 0;
    int mYBorderRadius = 0;
    int mBlurRadius = 0;
    int mPadding = 0;
    QColor mShadowColor = QColor(63, 63, 63, 180); // dark gray
    QColor mWindowBg = Qt::transparent;

    QImage mCacheShadow;
    bool mForceUpdateCache = false;

    QPainterPath mTransparentPath;
    QColor mTransparentBg = QColor(255, 255, 255, 127);
};
#endif // BORDERSHADOWEFFECT_H
