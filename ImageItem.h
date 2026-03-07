#pragma once
#include <QByteArray>
#include <QFileInfo>
#include <QMutex>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QSet>
#include <QSizeF>
#include <QString>

#include "ImageInfo.h"

class ImageItem {
public:
  ImageItem(QFileInfo fi) {
    m_imageinfo.fi = fi;
    m_imageinfo.m_imageitem = this;
  };

  QSizeF size, thumbsize;
  QPointF grid_idx;
  bool load_thumbnail = false;
  bool thumbnail_loaded = false;

  QRectF thumbrect() const;
  void preloadNext(bool haveit);
  void preloadSize(bool haveit);
  inline int idx() const { return m_idx; }
  inline void setIdx(int idx) { m_idx = idx; }
  void draw(QPainter &painter, QPointF mousepos);
  inline ImageInfo const &imageinfo() const { return m_imageinfo; }
  static void setXdim(int xdim) { m_xdim = xdim; }
  void setVisible(bool visible) { m_visible = visible; }

public slots:
  void setImage(QImage img);
  void setThumb(QImage thumb, QSize imgsize);

private:
  QPixmap m_img, m_thumbnail;
  QString errormessage;
  ImageInfo m_imageinfo;
  bool m_havebigimage_nextimage = false;
  bool m_havebigimage_size = false;
  int m_idx = -1;
  static inline int m_xdim = 0;
  bool m_visible = 0;
  bool m_requested_thumb = false;
  void haveBigImage();
  void haveThumbnail();
};
