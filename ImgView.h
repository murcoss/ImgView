#pragma once
#include <QFileDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <QMutex>
#include <QRunnable>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDirIterator>

struct ImgStruct {
    int idx;
    QString fn;
    QFileInfo fi;
    QPixmap img, thumbnail;
    QSize size;
    QString errormessage;
    QMutex mutex;
    enum WorkToDo { loadImage, createThumbnail, destroyImage};
    QSet<WorkToDo> worktodo;
};

class ImgLoaderTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    ImgStruct* imgStruct;
    static inline QMutex m_mutex;
    static inline QSet<ImgLoaderTask*> m_running;

    ImgLoaderTask(ImgStruct* s) : imgStruct(s){
        QMutexLocker locker(&m_mutex);
        setAutoDelete(true);
        m_running.insert(this);
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
    void loaded(size_t idx);
};


class ImgView : public QWidget {
    Q_OBJECT

    enum class FileDir { none, next, previous };

public:
    ImgView(QWidget* parent = 0);
    ~ImgView();

    void autofit();
    void loadImage(QStringList filename);
    void openFolder(QString dir);
    void loaded(size_t idx);
    void loadedFilenames(QList<ImgStruct*> is);
    static QSet<QString> const& supportedExtensions(){
        return m_supported_extensions;
    }

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

signals:
    void message(QString);

private:
    void customContextMenu(QPoint pos);
    void getFiles(QStringList filenames, QDirIterator::IteratorFlag itf);
    void btnWheelZoomIconUpdate();
    void nextImage(FileDir fd);
    void setTitle();
    void clearImages();

    static inline QSet<QString> m_supported_extensions;
    QList<ImgStruct*> m_allImages;
    QMutex m_allImage_mutex;
    ImgStruct* m_imgstruct = 0;
    QVector<QPushButton*> m_buttons;
    QPoint m_lastMousePos;
    QPointF m_offset;
    QPoint m_pressed;
    double m_zoom = 1.;
    bool m_show_overlay = false;
    bool m_show_text = false;
    bool m_wheel_zoom = true;
    bool m_show_thumb = false;
};

class DirIteratorTask : public QObject, public QRunnable {
    Q_OBJECT

    QStringList m_fns;
    QDirIterator::IteratorFlag m_itf;

public:
    DirIteratorTask(QStringList fns, QDirIterator::IteratorFlag itf)
        : m_fns(fns)
        , m_itf(itf){
        setAutoDelete(true);
    }

    void run() override;

signals:
    void loadedFilenames(QList<ImgStruct*> list);
};