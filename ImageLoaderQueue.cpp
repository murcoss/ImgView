#include "ImageLoaderQueue.h"
#include <QThread>
#include <QThreadPool>
#include "ImageHashStore.h"
#include "ImageItem.h"
#include "ImageLoaderTask.h"
#include "WorkItem.h"
#include "qstringview.h"

ImageLoaderQueue::ImageLoaderQueue() {
    m_imagehashstore = new ImageHashStore;
    QThread *dbThread = new QThread;
    m_imagehashstore->moveToThread(dbThread);

    QObject::connect(dbThread, &QThread::started, m_imagehashstore, &ImageHashStore::init);
    QObject::connect(dbThread, &QThread::finished, m_imagehashstore, &QObject::deleteLater);
    QObject::connect(this, &ImageLoaderQueue::requestThumbFromDatabase, m_imagehashstore, &ImageHashStore::requestThumb);
    dbThread->start();
}

void ImageLoaderQueue::insert(WorkItem wi) {
    if (wi.loadimage) {
        requestImage(wi);
    } else if (wi.loadthumb) {
        emit requestThumbFromDatabase(wi);
    }
}

void ImageLoaderQueue::setThumbFromDatabase(WorkItem wi, QImage thumb, QSize si) {
    if (!thumb.isNull()) {
        emit requestReady(wi, QImage(), thumb, si);
    }

    requestImage(wi);
}

void ImageLoaderQueue::requestImage(WorkItem wi) {
    ImageLoaderTask *ilt = new ImageLoaderTask(wi);
    connect(ilt, &ImageLoaderTask::loaded, this, [this](WorkItem wi, QImage img, QImage thumb, QSize si) { emit requestReady(wi, img, thumb, si); }, Qt::QueuedConnection);
    connect(ilt, &ImageLoaderTask::loadedThumbData, m_imagehashstore, &ImageHashStore::insertThumb, Qt::QueuedConnection);

    QThreadPool::globalInstance()->start(ilt);
}
