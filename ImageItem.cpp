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
};

QByteArray ImageItem::hash() const {
    return m_imageinfo.m_hash;
}

QRectF ImageItem::thumbrect() const {
    QPointF topLeft(grid_idx.x(), grid_idx.y());
    QRectF const rect(topLeft, QSizeF(1, 1));
    return rect;
}

void ImageItem::preloadNext(bool haveit) {
    m_havebigimage_nextimage = haveit;
    haveBigImage();
}

void ImageItem::preloadSize(bool haveit) {
    m_havebigimage_size = haveit;
    haveBigImage();
}

void ImageItem::haveBigImage() {
    bool const haveit = m_havebigimage_nextimage || m_havebigimage_size;
    if (haveit) {
        if (m_img.isNull()) {
            WorkItem wi = m_imageinfo;
            wi.loadimage = true;
            if (!m_requested_thumb) {
                wi.loadthumb = true;
                m_requested_thumb = true;
            }
            emit requestImageData(wi);
        }
    } else {
        m_img = QPixmap();
    }
}

void ImageItem::haveThumbnail() {
    if (!m_requested_thumb) {
        WorkItem wi = m_imageinfo;
        wi.loadthumb = true;
        emit requestImageData(wi);
        m_requested_thumb = true;
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
        return;
    }
    QRectF rect = thumbrect();
    QPen pen = p.pen();
    pen.setCosmetic(true);
    p.setPen(pen);

    // Get thumbnail
    if (m_thumbnail.isNull()) {
        haveThumbnail();
    }

    QTransform t = p.worldTransform();
    QRectF logicalRect = t.mapRect(QRectF(rect));
    preloadSize(logicalRect.width() > 256);

    // Draw rect or thumb or real image
    if (!m_img.isNull()) {
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
    if (rect.contains(mousepos)) {
        p.setPen(QPen(Qt::red, 0));
        QTransform inv = t.inverted();
        logicalRect.adjust(1, 1, -1, -1);
        QRectF deviceRect = inv.mapRect(logicalRect);
        p.drawRect(deviceRect);
    }
}
