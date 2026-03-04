#include <QObject>

#include "ImageInfo.h"
#include "ImageItem.h"
#include "qmutex.h"
#include <deque>

class ImageLoaderQueue : public QObject {
  static inline QMutex m_set_mutex;
  static inline int m_num_running = 0;
  static inline int m_cores = 1;

public:
  ImageLoaderQueue() = delete; // only static use
  static void insert(ImageInfo);
};