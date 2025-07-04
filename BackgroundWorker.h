#pragma once
#include <QRunnable>
#include <QDirIterator>
#include <QImageReader>
#include <QFileInfo>
#include <QPixmap>
#include <QMutex>
#include <QSet>

#include "ImageHashStore.h"

struct ImgStruct {
    int idx;
    QString fn;
    QFileInfo fi;
    QPixmap img, thumbnail;
    QSizeF size, thumbsize;
    QString errormessage;
    QMutex mutex;
    QPoint grid_idx;
    enum class WorkItem { none, loadImage, createThumbnail, destroyImage };
    QSet<WorkItem> worktodo;
    bool load_thumbnail = false;
    bool thumbnail_loaded = false;
};

class ImgLoaderTask : public QObject, public QRunnable {
    Q_OBJECT

    void readImageData(QString const filename, QByteArray& imageData);
    void readImage(QByteArray & imageData, QImage& image);

public:
    ImgStruct* imgStruct;
    static inline ImageHashStore* m_imagehashstore = nullptr;
    static inline QMutex m_mutex;
    static inline QSet<ImgLoaderTask*> m_running;

    ImgLoaderTask(ImgStruct* s)
        : imgStruct(s)
    {
        QMutexLocker locker(&m_mutex);
        setAutoDelete(true);
        m_running.insert(this);
        if (!m_imagehashstore) {
            m_imagehashstore = new ImageHashStore;
        }
    }
    ~ImgLoaderTask()
    {
        QMutexLocker locker(&m_mutex);
        m_running.remove(this);
    }
    static qsizetype runningCount()
    {
        QMutexLocker locker(&m_mutex);
        return m_running.size();
    }

    void run() override;

signals:
    void loaded(ImgStruct::WorkItem wr, ImgStruct * imagestruct);
};

class DirIteratorTask : public QObject, public QRunnable {
    Q_OBJECT

    QStringList m_fns;
    QDirIterator::IteratorFlag m_itf;
    static inline QSet<QString> m_supported_extensions;

public:
    DirIteratorTask(QStringList fns, QDirIterator::IteratorFlag itf)
        : m_fns(fns)
        , m_itf(itf)
    {
        setAutoDelete(true);
    }

    static inline QSet<QString> const& supportedExtensions(){
        if (m_supported_extensions.isEmpty()) {
            QByteArrayList const bal = QImageReader::supportedImageFormats();
            m_supported_extensions.clear();
            for (auto const& b : bal) {
                m_supported_extensions.insert(b.toLower());
            }
            if (m_supported_extensions.contains("svg")) {
                m_supported_extensions.remove("svg");
            }
            if (m_supported_extensions.contains("ico")) {
                m_supported_extensions.remove("ico");
            }
        }
        return m_supported_extensions;
    }

    void run() override;

signals:
    void loadedFilenames(QList<ImgStruct*> list);
};