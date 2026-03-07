#include <QMutexLocker>
#include <QPainter>
#include <QPixmap>

#include "ImageInfo.h"
#include "ImageItem.h"
#include "ImageLoaderQueue.h"

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
      m_imageinfo.worktype = ImageInfo::WorkType::loadImage;
      ImageLoaderQueue::insert(m_imageinfo);
    }
  } else {
    m_img = QPixmap();
  }
}

void ImageItem::haveThumbnail() {
  if (!m_requested_thumb) {
    ImageInfo ii = m_imageinfo;
    ii.worktype = ImageInfo::WorkType::loadThumbnail256;
    ImageLoaderQueue::insert(ii);
    m_requested_thumb = true;
  }
}

void ImageItem::setImage(QImage img) {
  m_img = QPixmap::fromImage(img);
  size = img.size();
}

void ImageItem::setThumb(QImage thumb, QSize imgsize) {
  m_thumbnail = QPixmap::fromImage(thumb);
  thumbsize =
      thumb.size().toSizeF().scaled(QSizeF(1., 1.), Qt::KeepAspectRatio);
  size = imgsize;
}

void ImageItem::draw(QPainter &p, QPointF mousepos) {
  if (!m_visible) {
    return;
  }
  QRectF const rect = thumbrect();
  QPen pen = p.pen();
  pen.setCosmetic(true);
  p.setPen(pen);
  QRect deviceArea = p.viewport();
  double wi = deviceArea.width();
  double he = deviceArea.height();

  // Get thumbnail
  if (m_thumbnail.isNull()) {
    haveThumbnail();
  }

  QTransform t = p.worldTransform();
  QTransform inv = t.inverted();
  QRectF logicalRect = t.mapRect(QRectF(rect));
  preloadSize(logicalRect.width() > 256);

  // Draw rect or thumb or real image
  if (!m_img.isNull()) {
    p.drawPixmap(rect, m_img, QRectF(QPointF(0, 0), m_img.size()));
  } else if (!m_thumbnail.isNull()) {
    p.drawPixmap(rect, m_thumbnail, QRectF(QPointF(0, 0), m_thumbnail.size()));
  } else {
    p.setPen(QPen(Qt::black, 0));
    p.drawRect(rect);
  }

  // draw red rect around hovered image
  if (rect.contains(mousepos)) {
    p.setPen(QPen(Qt::red, 0));
    QTransform inv = t.inverted();
    logicalRect.adjust(-1, -1, 1, 1);
    QRectF deviceRect = inv.mapRect(logicalRect);
    p.drawRect(deviceRect);
  }
}