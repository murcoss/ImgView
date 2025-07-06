#include <QFile>
#include <QBuffer>
#include <QFileInfo>
#include <QPixmap>
#include <QImageReader>

#include "BackgroundWorker.h"


void DirIteratorTask::run()
{
    QElapsedTimer ti;
    ti.start();

    QList<ImageItem*> tmpstruct;

    if (m_fns.isEmpty()) {
        return;
    }

    // Read file list as a list of file
    QString filetoshowfirst;
    for (auto const& fn : m_fns) {
        QFileInfo fi(fn);
        if (supportedExtensions().contains(fi.suffix().toLower())) {
            ImageItem* image_item = new ImageItem;
            tmpstruct.push_back(image_item);
            image_item->fn = fn;
            image_item->fi = fi;
            filetoshowfirst = fn;
        }
    }

    if (!tmpstruct.isEmpty()) {
        emit loadedFilenames(tmpstruct);
        tmpstruct.clear();
    }

    // If we got a list of files, only load these
    if ((m_fns.size() > 1) && !QFileInfo(m_fns.front()).isDir()) {
        return;
    }

    // If it is a directory or only one file, iterate the directory
    QFileInfo fi(m_fns.front());
    QString dir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    QDirIterator it(dir, QDir::Files, m_itf);
    while (it.hasNext()) {
        QString const file = it.next();
        if (file != filetoshowfirst) {
            QFileInfo fi(file);
            if (DirIteratorTask::supportedExtensions().contains(fi.suffix().toLower())) {
                tmpstruct.push_back(new ImageItem);
                tmpstruct.back()->fn = file;
                tmpstruct.back()->fi = fi;
            }
        }

        if ((ti.elapsed() > 10) || !it.hasNext()) {
            ti.restart();
            emit loadedFilenames(tmpstruct);
            tmpstruct.clear();
        }
    }
}

void ImgLoaderTask::readImageData(QString const filename, QByteArray& imageData){
    if (imageData.isEmpty()) {
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            imageData = file.readAll();
            file.close();
        } else {
            QMutexLocker locker(&m_image_item->mutex);
            m_image_item->errormessage = file.errorString();
        }
    }
}

void ImgLoaderTask::readImage(QByteArray & imageData, QImage& image){
    if (image.isNull()) {
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer);
        if (!reader.read(&image)){
            QMutexLocker locker(&m_image_item->mutex);
            m_image_item->errormessage = reader.errorString();
        }
    }
}

void ImgLoaderTask::run()
{
    QElapsedTimer ti;
    ti.start();

    if (!m_image_item) {
        return;
    }

    m_image_item->mutex.lock();
    QSet<ImageItem::WorkItem> worktodo = std::move(m_image_item->worktodo);
    QFileInfo const fi = m_image_item->fi;
    m_image_item->mutex.unlock();

    QImage image, thumb;
    QByteArray imageData, hash;

    if (worktodo.contains(ImageItem::WorkItem::loadImage)) {
        worktodo.insert(ImageItem::WorkItem::createThumbnail);
        readImageData(fi.absoluteFilePath(), imageData);
        if (!imageData.isEmpty()){
            readImage(imageData, image);
            if (!image.isNull()) {
                QMutexLocker locker(&m_image_item->mutex);
                m_image_item->img = std::move(QPixmap::fromImage(image));
                m_image_item->size = image.size();
                emit loaded(ImageItem::WorkItem::loadImage, m_image_item);
            }
        }
    }

    if (worktodo.contains(ImageItem::WorkItem::createThumbnail)) {
        readImageData(fi.absoluteFilePath(), imageData);
        if (!imageData.isEmpty()){
            hash = ImageHashStore::calculateHash(imageData);
            thumb = m_imagehashstore->get(hash);
        }
        if (!imageData.isEmpty() && thumb.isNull()) {
            readImage(imageData, image);
            if (!image.isNull()) {
                hash = ImageHashStore::calculateHash(imageData);
                int const constexpr maxsize = 256;
                int const imgsize = std::max(image.width(), image.height());
                if (imgsize < maxsize) {
                    thumb = image;
                } else {
                    thumb = image.scaled(QSize(maxsize, maxsize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
                m_imagehashstore->insert(thumb, hash);
            }
        }
        if (!thumb.isNull()) {
            QMutexLocker locker(&m_image_item->mutex);
            m_image_item->thumbnail = QPixmap::fromImage(thumb);
            m_image_item->size = image.size();
            if (m_image_item->size.width() > 0) {
                m_image_item->thumbsize = m_image_item->size.scaled(QSizeF(1., 1.), Qt::KeepAspectRatio);
            } else {
                m_image_item->thumbsize = thumb.size().toSizeF().scaled(QSizeF(1., 1.), Qt::KeepAspectRatio);
            }
            emit loaded(ImageItem::WorkItem::createThumbnail, m_image_item);
        }
    }

    if (m_image_item->worktodo.contains(ImageItem::WorkItem::destroyImage)) {
        QMutexLocker locker(&m_image_item->mutex);
        QPixmap().swap(m_image_item->img);
        emit loaded(ImageItem::WorkItem::destroyImage, m_image_item);
    }

    return;
}