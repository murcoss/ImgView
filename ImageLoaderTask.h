#pragma once
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QMutex>
#include <QPixmap>
#include <QRunnable>

#include <QSet>
#include <qobject.h>

#include "ImageHashStore.h"
#include "ImageInfo.h"

class ImageLoaderTask : public QObject, public QRunnable {
  Q_OBJECT

private:
  void readImageData(QString const filename, QByteArray &imageData);
  void readImage(QByteArray &imageData, QImage &image);

  ImageInfo m_imageinfo;

  static inline ImageHashStore *m_imagehashstore = nullptr;

public:
  ImageLoaderTask(ImageInfo wi);
  static int runningCount();

  void run() override;

signals:
  void loadedThumb(QImage thumb, QSize imagesize);
  void loadedImage(QImage img);
};
