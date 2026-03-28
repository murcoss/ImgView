#include <QMutexLocker>
#include <QPainter>
#include <QPixmap>

#include "ImageItem.h"
#include "ImageLoaderQueue.h"
#include "WorkItem.h"

ImageItem::ImageItem(QFileInfo fi) {
    m_imageinfo.fi = fi;
    QString const textkey = QStringLiteral(u"path=%1;size=%2;time=%3")
                                .arg(m_imageinfo.fi.absoluteFilePath())
                                .arg(m_imageinfo.fi.size())
                                .arg(m_imageinfo.fi.lastModified().toSecsSinceEpoch());
    m_imageinfo.m_hash = QCryptographicHash::hash(textkey.toUtf8(), QCryptographicHash::Sha256);

    WorkItem wi = m_imageinfo;
    wi.loadthumb = true;
    emit requestImageData(wi);
};

void ImageItem::preloadNext(bool haveit) {
    m_havebigimage_nextimage = haveit;
    haveBigImage();
}

void ImageItem::preloadSize(bool haveit) {
    m_havebigimage_size = haveit;
    haveBigImage();
}

void ImageItem::haveBigImage() {
    bool const haveit = m_havebigimage_nextimage || (m_havebigimage_size && m_visible);
    if (haveit) {
        if (m_img.isNull()) {
            m_imageinfo.loadimage = true;
            m_imageinfo.loadthumb = true;
            emit requestImageData(m_imageinfo);
        }
    } else {
        m_imageinfo.loadimage = false;
        if (!m_img.isNull()) {
            m_img = QPixmap();
        }
    }
}

void ImageItem::setImage(WorkItem, QImage img) {
    m_img = QPixmap::fromImage(img);
    size = img.size().toSizeF().scaled(QSizeF(1., 1.), Qt::KeepAspectRatio);
}

void ImageItem::setThumb(WorkItem wi, QImage thumb, QSize imgsize) {
    m_thumbnail = QPixmap::fromImage(thumb);
    thumbsize = thumb.size().toSizeF().scaled(QSizeF(1., 1.), Qt::KeepAspectRatio);
    size = imgsize.toSizeF().scaled(QSizeF(1., 1.), Qt::KeepAspectRatio);
    qDebug() << "got thumb for " << wi.fi.fileName() << "hash: " << hash();
}

void ImageItem::draw(QPainter &p, QPointF mousepos) {
    if (!m_visible) {
        haveBigImage();
        return;
    }
    QRectF rect = thumbrect();
    QPen pen = p.pen();
    pen.setCosmetic(true);
    p.setPen(pen);

    QTransform t = p.worldTransform();
    QRectF logicalRect = t.mapRect(QRectF(rect));
    preloadSize(logicalRect.width() > 256);

    bool const undermouse = rect.contains(mousepos);

    // Draw rect or thumb or real image
    if (undermouse && !m_img.isNull()) {
        rect.setSize(size);
        p.drawPixmap(rect, m_img, QRectF(QPointF(0, 0), m_img.size()));
    } else if (!m_thumbnail.isNull()) {
        rect.setSize(thumbsize);
        p.drawPixmap(rect, m_thumbnail, QRectF(QPointF(0, 0), m_thumbnail.size()));
    } else {
        p.setPen(QPen(Qt::black, 0));
        p.drawRect(rect);
    }

    // draw red rect around hovered image
    if (undermouse) {
        p.setPen(QPen(Qt::red, 0));
        QTransform inv = t.inverted();
        logicalRect.adjust(1, 1, -1, -1);
        QRectF deviceRect = inv.mapRect(logicalRect);
        p.drawRect(deviceRect);
    }
}
