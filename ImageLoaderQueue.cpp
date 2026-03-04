#include "ImageLoaderQueue.h"
#include "ImageInfo.h"
#include "ImageLoaderTask.h"
#include "qmutex.h"
#include <QThread>
#include <QThreadPool>

void ImageLoaderQueue::insert(ImageInfo info) {
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
        }
      },
      Qt::QueuedConnection);
  QThreadPool::globalInstance()->start(ilt);
}