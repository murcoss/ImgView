#include "ImageLoaderQueue.h"
#include "ImageHashStore.h"
#include "ImageInfo.h"
#include "ImageItem.h"
#include "ImageLoaderTask.h"
#include "qstringview.h"
#include <QThread>
#include <QThreadPool>

QByteArray ImageLoaderQueue::generatehash(QFileInfo const fi) {
  QString const textkey = QStringLiteral(u"path=%1;size=%2;time=%3")
                              .arg(fi.absoluteFilePath())
                              .arg(fi.size())
                              .arg(fi.lastModified().toSecsSinceEpoch());
  QByteArray const hash =
      QCryptographicHash::hash(textkey.toUtf8(), QCryptographicHash::Sha256);
  return hash;
}

void ImageLoaderQueue::insert(ImageInfo const &info) {
  if (!m_imagehashstore) {
    m_imagehashstore = new ImageHashStore;
  }

  if (info.worktype == ImageInfo::WorkType::loadThumbnail256) {
    QByteArray const hash = generatehash(info.fi);

    QByteArray const thumbdata = m_imagehashstore->getByHash(hash);
    QImage thumb;
    thumb.loadFromData(thumbdata, "JPEG");
    if (!thumb.isNull()) {
      static_cast<ImageItem *>(info.m_imageitem)->setThumb(thumb, thumb.size());
      return;
    }
  }

  ImageLoaderTask *ilt = new ImageLoaderTask(info);
  connect(
      ilt, &ImageLoaderTask::loadedImage, ilt,
      [info](QImage img) {
        if (info.m_imageitem) {
          static_cast<ImageItem *>(info.m_imageitem)->setImage(img);
        }
      },
      Qt::QueuedConnection);
  connect(
      ilt, &ImageLoaderTask::loadedThumb, ilt,
      [info](QImage thumb, QSize imgsize) {
        if (info.m_imageitem) {
          static_cast<ImageItem *>(info.m_imageitem)->setThumb(thumb, imgsize);
          insertThumb(thumb, info.fi);
        }
      },
      Qt::QueuedConnection);
  QThreadPool::globalInstance()->start(ilt);
}

void ImageLoaderQueue::insertThumb(QImage const thumb, QFileInfo const fi) {
  QByteArray const hash = generatehash(fi);
  m_imagehashstore->insert(thumb, hash, fi.absoluteFilePath(), fi.size());
}