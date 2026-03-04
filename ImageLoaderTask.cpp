#include <QBuffer>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QPixmap>
#include <QThreadPool>

#include "ImageInfo.h"
#include "ImageLoaderTask.h"
#include "qstringview.h"

void ImageLoaderTask::readImageData(QString const filename,
                                    QByteArray &imageData) {
  if (imageData.isEmpty()) {
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
      imageData = file.readAll();
      file.close();
    } else {
      m_imageinfo.m_error_message = file.errorString();
    }
  }
}

void ImageLoaderTask::readImage(QByteArray &imageData, QImage &image) {
  if (image.isNull()) {
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    if (!reader.read(&image)) {
      m_imageinfo.m_error_message = reader.errorString();
    }
  }
}

void ImageLoaderTask::run() {
  QImage image, thumb;
  QByteArray imageData;

  if (m_imageinfo.worktype == ImageInfo::WorkType::loadThumbnail256) {
    QString const textkey =
        QStringLiteral(u"path=%1;size=%2;time=%3")
            .arg(m_imageinfo.fi.absoluteFilePath())
            .arg(m_imageinfo.fi.size())
            .arg(m_imageinfo.fi.lastModified().toSecsSinceEpoch());
    QByteArray const hash =
        QCryptographicHash::hash(textkey.toUtf8(), QCryptographicHash::Sha256);

    QByteArray const thumbdata = m_imagehashstore->getByHash(hash);
    thumb.loadFromData(thumbdata, "JPEG");

    if (thumb.isNull()) {
      readImageData(m_imageinfo.fi.absoluteFilePath(), imageData);
    }
    if (!imageData.isEmpty() && thumb.isNull()) {
      readImage(imageData, image);
      if (!image.isNull()) {
        int const constexpr maxsize = 256;
        int const imgsize = std::max(image.width(), image.height());
        if (imgsize < maxsize) {
          thumb = image;
        } else {
          thumb = image.scaled(QSize(maxsize, maxsize), Qt::KeepAspectRatio,
                               Qt::FastTransformation);
        }
        m_imagehashstore->insert(thumb, hash, m_imageinfo.fi.absoluteFilePath(),
                                 m_imageinfo.fi.size());
      }
    }

    int siw = thumb.width();
    int sih = thumb.height();

    emit loadedThumb(std::move(thumb), image.size());
  } else if (m_imageinfo.worktype == ImageInfo::WorkType::loadImage) {
    QImage const img(m_imageinfo.fi.absoluteFilePath());
    emit loadedImage(std::move(img));
  }
}

ImageLoaderTask::ImageLoaderTask(ImageInfo info) {
  setAutoDelete(true);

  m_imageinfo = info;

  if (!m_imagehashstore) {
    m_imagehashstore = new ImageHashStore;
  }
}
