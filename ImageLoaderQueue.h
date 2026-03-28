#include <QObject>

#include "ImageHashStore.h"
#include "WorkItem.h"
#include "qmutex.h"

class ImageLoaderQueue : public QObject {
    Q_OBJECT
public:
    ImageLoaderQueue();
    void insert(WorkItem wi);
    void requestImage(WorkItem wi);
    void setThumbFromDatabase(WorkItem wi, QImage thumb, QSize si);

signals:
    void requestThumbFromDatabase(WorkItem wi);
    void requestReady(WorkItem wi, QImage image, QImage thumb, QSize si);

private:
    QMutex m_set_mutex;
    int m_num_running = 0;
    QByteArray generatehash(QFileInfo const fi);
    ImageHashStore *m_imagehashstore = nullptr;
};
