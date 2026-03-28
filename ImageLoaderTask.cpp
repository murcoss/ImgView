#include <QBuffer>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QPixmap>
#include <QThreadPool>

#include "ImageLoaderTask.h"
#include "WorkItem.h"
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
    QSize si;
    QByteArray imageData;

    qDebug() << "reading " << m_imageinfo.fi.absoluteFilePath();

    if (m_imageinfo.loadimage) {
        image = QImage(m_imageinfo.fi.absoluteFilePath());
        si = image.size();
    }

<<<<<<< HEAD
    emit loadedThumb(std::move(thumb), image.size());
  } else if (m_imageinfo.worktype == ImageInfo::WorkType::loadImage) {
    QImage img(m_imageinfo.fi.absoluteFilePath());
    emit loadedImage(std::move(img));
  }
=======
    if (m_imageinfo.loadthumb) {
        if (image.isNull()) {
            QImageReader reader(m_imageinfo.fi.absoluteFilePath());
            si = reader.size();
            QSize scaledSize = (std::max(reader.size().width(), reader.size().height()) < 256) ? reader.size() : QSize(256, 256);
            reader.setScaledSize(scaledSize);
            thumb = reader.read();
        } else {
            int const maxsize = 256;
            int const imgsize = std::max(image.width(), image.height());
            thumb = (imgsize <= maxsize) ? image : image.scaled(QSize(maxsize, maxsize), Qt::KeepAspectRatio, Qt::FastTransformation);
        }

        QByteArray buffer;
        QBuffer qbuffer(&buffer);
        qbuffer.open(QIODevice::WriteOnly);
        thumb.save(&qbuffer, "WEBP", 80);
        emit loadedThumbData(m_imageinfo, std::move(buffer));
    }

    emit loaded(m_imageinfo, std::move(image), std::move(thumb), si);
>>>>>>> d9f50c8 (wip)
}

ImageLoaderTask::ImageLoaderTask(WorkItem info) {
    setAutoDelete(true);
    m_imageinfo = info;
}
