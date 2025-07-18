#include <QFile>
#include <QBuffer>
#include <QFileInfo>
#include <QPixmap>
#include <QImageReader>
#include <QThreadPool>

#include "BackgroundWorker.h"
#include "ImgView.h"


void DirIteratorTask::run()
{
    QElapsedTimer ti;
    ti.start();

    QList<ImageItem*> newimageitems;

    if (m_fns.isEmpty()) {
        return;
    }

    // Read file list as a list of file
    QString filetoshowfirst;
    for (auto const& fn : m_fns) {
        QFileInfo fi(fn);
        if (supportedExtensions().contains(fi.suffix().toLower())) {
            ImageItem* image_item = new ImageItem;
            newimageitems.push_back(image_item);
            image_item->fi = fi;
            filetoshowfirst = fn;
        }
    }

    if (!newimageitems.isEmpty()) {
        emit loadedFilenames(newimageitems);
        newimageitems.clear();
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
                newimageitems.push_back(new ImageItem);
                newimageitems.back()->fi = fi;
            }
        }

        if ((ti.elapsed() > 10) || !it.hasNext()) {
            ti.restart();
            emit loadedFilenames(newimageitems);
            newimageitems.clear();
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

void ImgLoaderTask::run(){
    QElapsedTimer ti;
    ti.start();

    if (!m_image_item) {
        return;
    }

    m_image_item->mutex.lock();
    QSet<ImageItem::WorkItem> worktodo = m_image_item->worktodo;
    QFileInfo const fi = m_image_item->fi;
    QByteArray hash = m_image_item->hash;
    m_image_item->mutex.unlock();

    QImage image, thumb;
    QByteArray imageData;

    if (worktodo.contains(ImageItem::WorkItem::loadImage)) {
        if (!m_image_item->thumbnail.isNull()) {
            worktodo.insert(ImageItem::WorkItem::loadThumbnail256);
        }
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

    if (worktodo.contains(ImageItem::WorkItem::loadThumbnail256)) {
        if (hash.isEmpty()) {
            QString const textkey = QStringLiteral(u"path=%1;size=%2;time=%3").arg(fi.absoluteFilePath()).arg(fi.size()).arg(fi.lastModified().toSecsSinceEpoch());
            hash = QCryptographicHash::hash(textkey.toUtf8(), QCryptographicHash::Sha256);
        }
        QByteArray const thumbdata = m_imagehashstore->getByHash(hash);
        thumb.loadFromData(thumbdata, "JPEG");

        if (!thumb.isNull()) {
            //qDebug() << "thumb loaded by textkey " << m_image_item->idx;
        }
        if (thumb.isNull()) {
            readImageData(fi.absoluteFilePath(), imageData);
        }
        if (!imageData.isEmpty() && thumb.isNull()) {
            readImage(imageData, image);
            if (!image.isNull()) {
                int const constexpr maxsize = 256;
                int const imgsize = std::max(image.width(), image.height());
                if (imgsize < maxsize) {
                    thumb = image;
                } else {
                    thumb = image.scaled(QSize(maxsize, maxsize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
                m_imagehashstore->insert(thumb, hash, fi.absoluteFilePath(), fi.size());
                //qDebug() << "created thumb " << m_image_item->idx;
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
            emit loaded(ImageItem::WorkItem::loadThumbnail256, m_image_item);
        }
    }

    if (m_image_item->worktodo.contains(ImageItem::WorkItem::destroyImage)) {
        QMutexLocker locker(&m_image_item->mutex);
        QPixmap().swap(m_image_item->img);
        emit loaded(ImageItem::WorkItem::destroyImage, m_image_item);
    }

    QMutexLocker locker(&m_image_item->mutex);
    m_image_item->worktodo.clear();
    m_image_item->hash = std::move(hash);

    return;
}

ImgLoaderTask::ImgLoaderTask(QObject* parent){
    QMutexLocker locker(&m_mutex);
    setAutoDelete(true);
    m_running.insert(this);
    if (ImgView * imgview = qobject_cast<ImgView*>(parent)) {
        connect(this, &ImgLoaderTask::loaded, imgview, &ImgView::loaded);
    }
    if (!m_imagehashstore) {
        m_imagehashstore = new ImageHashStore;
    }
}

ImgLoaderTask::ImgLoaderTask(ImageItem* s, QObject * parent):ImgLoaderTask(parent){
    m_image_item = s;
    QThreadPool::globalInstance()->start(this);
}

ImgLoaderTask::ImgLoaderTask(QSet<ImageItem*> items, QObject * parent):ImgLoaderTask(parent){
    m_image_items = std::move(items);
    QThreadPool::globalInstance()->start(this);
}

ImgLoaderTask::~ImgLoaderTask(){
    QMutexLocker locker(&m_mutex);
    m_running.remove(this);
}

qsizetype ImgLoaderTask::runningCount(){
    QMutexLocker locker(&m_mutex);
    return m_running.size();
}