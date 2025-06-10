#pragma once
#include <QIconEngine>
#include <QSvgGenerator>
#include <QPainter>

class IconEngine : public QIconEngine {
public:
    inline IconEngine() {
        // DO NOT DELETE, THIS WAY YOU CAN GENERATE THE APPLICATION ICON; JUST CUSTOMIZE THIS CODE
        //QList<int> sizes = { 16, 24, 32, 48, 64, 128, 256, 512};
        //for (int size : sizes) {
        //    QImage image(size, size, QImage::Format_ARGB32);
        //    image.fill(Qt::transparent); // Ensure background is transparent

        //    QPainter painter(&image);
        //    m_col = Qt::darkGreen; // Or any other color setting
        //    paint(&painter, QRect(0, 0, size, size), QIcon::Normal, QIcon::On);
        //    painter.end();

        //    QString filename = QString("ImgViewLogo_%1.png").arg(size);
        //    image.save(filename);
        //}

    };
    inline ~IconEngine() {};

    virtual void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state);
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state);
    inline static void setColor(QColor col){
        m_col = col;
    }
    inline virtual QIconEngine* clone(void) const{
        return new IconEngine;
    }

private:
    static inline QColor m_col;
};