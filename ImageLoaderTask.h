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
#include "WorkItem.h"

class ImageLoaderTask : public QObject, public QRunnable {
  Q_OBJECT

 public:
  ImageLoaderTask(WorkItem wi);
  static int runningCount();

  void run() override;

private:
  void readImageData(QString const filename, QByteArray &imageData);
  void readImage(QByteArray &imageData, QImage &image);

  WorkItem m_imageinfo;

signals:
    void loaded(WorkItem wi, QImage img, QImage thumb, QSize si);
    void loadedThumbData(WorkItem wi, QByteArray thumbdata, QSize si);
};
