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

#include "WorkItem.h"

class ImageItem : public QObject
{
    Q_OBJECT
public:
    ImageItem(QObject *p)
        : QObject(p) {
    }
    ImageItem(QFileInfo fi);

    QSizeF size, thumbsize;
    bool load_thumbnail = false;
    bool thumbnail_loaded = false;

    QRectF const &thumbrect() const {
        return m_thumbrect;
    };
    void preloadNext(bool haveit);
    void preloadSize(bool haveit);
    inline int idx() const {
        return m_idx;
    }
    inline void setIdx(int idx, QPoint p) {
        m_idx = idx;
        grid_idx = p;
        m_thumbrect = QRectF(QPointF(grid_idx.x(), grid_idx.y()), QSizeF(1., 1.));
    }
    void draw(QPainter &painter, QPointF mousepos);
    inline WorkItem const &imageinfo() const { return m_imageinfo; }
    static void setXdim(int xdim) { m_xdim = xdim; }
    inline void setVisible(bool visible) {
        m_visible = visible;
    }
    QByteArray const &hash() const {
        return m_imageinfo.m_hash;
    }
    inline bool isUnderMouse(QPointF mousepos) const {
        return m_thumbrect.contains(mousepos);
    }

public slots:
    void setImage(WorkItem wi, QImage img);
    void setThumb(WorkItem wi, QImage thumb, QSize imgsize);

private:
    QPixmap m_img, m_thumbnail;
    QString errormessage;
    QPointF grid_idx;
    QRectF m_thumbrect;
    WorkItem m_imageinfo;
    bool m_havebigimage_nextimage = false;
    bool m_havebigimage_size = false;
    int m_idx = -1;
    static inline int m_xdim = 0;
    bool m_visible = 0;
    void haveBigImage();
signals:
    void requestImageData(WorkItem);
};
