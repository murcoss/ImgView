#include <QObject>

#include "ImageHashStore.h"
#include "ImageInfo.h"
#include "qmutex.h"

class ImageLoaderQueue : public QObject {
  static inline QMutex m_set_mutex;
  static inline int m_num_running = 0;
  static inline ImageHashStore *m_imagehashstore = nullptr;
  static QByteArray generatehash(QFileInfo const fi);

public:
  ImageLoaderQueue() = delete; // only static use
  static void insert(ImageInfo const &info);
  static void insertThumb(QImage const thumb, QFileInfo const fi);
};