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

    QList<ImgStruct*> tmpstruct;

    if (m_fns.isEmpty()) {
        return;
    }

    // Read file list as a list of file
    QString filetoshowfirst;
    int idx = 0;
    for (auto const& fn : m_fns) {
        QFileInfo fi(fn);
        if (supportedExtensions().contains(fi.suffix().toLower())) {
            tmpstruct.push_back(new ImgStruct);
            tmpstruct.back()->fn = fn;
            tmpstruct.back()->fi = fi;
            tmpstruct.back()->idx = idx++;
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
                tmpstruct.push_back(new ImgStruct);
                tmpstruct.back()->fn = file;
                tmpstruct.back()->fi = fi;
                tmpstruct.back()->idx = idx++;
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
            QMutexLocker locker(&imgStruct->mutex);
            imgStruct->errormessage = file.errorString();
        }
    }
}

void ImgLoaderTask::readImage(QByteArray & imageData, QImage& image){
    if (image.isNull()) {
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer);
        if (!reader.read(&image)){
            QMutexLocker locker(&imgStruct->mutex);
            imgStruct->errormessage = reader.errorString();
        }
    }
}

void ImgLoaderTask::run()
{
    QElapsedTimer ti;
    ti.start();

    if (!imgStruct) {
        return;
    }

    imgStruct->mutex.lock();
    QSet<ImgStruct::WorkItem> worktodo = std::move(imgStruct->worktodo);
    QFileInfo const fi = imgStruct->fi;
    imgStruct->mutex.unlock();

    QImage image, thumb;
    QByteArray imageData, hash;

    if (worktodo.contains(ImgStruct::WorkItem::loadImage)) {
        worktodo.insert(ImgStruct::WorkItem::createThumbnail);
        readImageData(fi.absoluteFilePath(), imageData);
        if (!imageData.isEmpty()){
            readImage(imageData, image);
            if (!image.isNull()) {
                QMutexLocker locker(&imgStruct->mutex);
                imgStruct->img = std::move(QPixmap::fromImage(image));
                imgStruct->size = image.size();
                emit loaded(ImgStruct::WorkItem::loadImage, imgStruct);
            }
        }
    }

    if (worktodo.contains(ImgStruct::WorkItem::createThumbnail)) {
        readImageData(fi.absoluteFilePath(), imageData);
        if (!imageData.isEmpty()){
            hash = ImageHashStore::calculateHash(imageData);
            thumb = m_imagehashstore->get(hash);
        }
        if (thumb.isNull()) {
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
            QMutexLocker locker(&imgStruct->mutex);
            imgStruct->thumbnail = QPixmap::fromImage(thumb);
            imgStruct->size = image.size();
            if (imgStruct->size.width() > 0) {
                imgStruct->thumbsize = imgStruct->size.scaled(QSizeF(1., 1.), Qt::KeepAspectRatio);
            } else {
                imgStruct->thumbsize = thumb.size().toSizeF().scaled(QSizeF(1., 1.), Qt::KeepAspectRatio);
            }
            emit loaded(ImgStruct::WorkItem::createThumbnail, imgStruct);
        }
    }

    if (imgStruct->worktodo.contains(ImgStruct::WorkItem::destroyImage)) {
        QMutexLocker locker(&imgStruct->mutex);
        QPixmap().swap(imgStruct->img);
        emit loaded(ImgStruct::WorkItem::destroyImage, imgStruct);
    }

    return;
}