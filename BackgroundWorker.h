#pragma once
#include <QRunnable>
#include <QDirIterator>
#include <QImageReader>
#include <QFileInfo>
#include <QPixmap>
#include <QMutex>
#include <QSet>

#include "ImageHashStore.h"

struct ImageItem {
    enum class WorkItem { none, loadImage, createThumbnail, destroyImage };
    QString fn;
    QPixmap img, thumbnail;
    QString errormessage;
    QSizeF size, thumbsize;
    QFileInfo fi;
    QMutex mutex;
    QPoint grid_idx;
    QSet<WorkItem> worktodo;
    int idx;
    bool load_thumbnail = false;
    bool thumbnail_loaded = false;
};

class ImgLoaderTask : public QObject, public QRunnable {
    Q_OBJECT

    void readImageData(QString const filename, QByteArray& imageData);
    void readImage(QByteArray& imageData, QImage& image);

public:
    ImageItem* m_image_item;
    static inline ImageHashStore* m_imagehashstore = nullptr;
    static inline QMutex m_mutex;
    static inline QSet<ImgLoaderTask*> m_running;

    ImgLoaderTask(ImageItem* s)
        : m_image_item(s){
        QMutexLocker locker(&m_mutex);
        setAutoDelete(true);
        m_running.insert(this);
        if (!m_imagehashstore) {
            m_imagehashstore = new ImageHashStore;
        }
    }
    ~ImgLoaderTask(){
        QMutexLocker locker(&m_mutex);
        m_running.remove(this);
    }
    static qsizetype runningCount(){
        QMutexLocker locker(&m_mutex);
        return m_running.size();
    }

    void run() override;

signals:
    void loaded(ImageItem::WorkItem wr, ImageItem* image_item);
};

class DirIteratorTask : public QObject, public QRunnable {
    Q_OBJECT

    QStringList m_fns;
    QDirIterator::IteratorFlag m_itf;

public:
    DirIteratorTask(QStringList fns, QDirIterator::IteratorFlag itf)
        : m_fns(fns)
        , m_itf(itf)
    {
        setAutoDelete(true);
    }

    static const QSet<QString>& supportedExtensions(){
        static QSet<QString> extensions = [] {
            QSet<QString> ext;
            for (const auto& b : QImageReader::supportedImageFormats()) {
                if (b != "svg" && b != "ico") {
                    ext.insert(QString::fromLatin1(b).toLower());
                }
            }
            return ext;
        }();
        return extensions;
    }

    void run() override;

signals:
    void loadedFilenames(QList<ImageItem*> list);
};