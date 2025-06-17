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

struct ImgStruct {
    int idx;
    bool visible;
    QString fn;
    QFileInfo fi;
    QPixmap img;
    QMutex mutex;
};

class ImgLoaderTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    ImgStruct* imgStruct;

    ImgLoaderTask(ImgStruct* s) : imgStruct(s){
        setAutoDelete(true);
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
    void loaded(size_t idx);

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
    void btnWheelZoomIconUpdate();
    void nextImage(FileDir fd);
    void setTitle();

    QSet<QString> m_supported_extensions;
    QList<ImgStruct*> m_allImages;
    QVector<QPushButton*> m_buttons;
    QPoint m_lastMousePos;
    QPointF m_offset;
    QPoint m_pressed;
    double m_zoom = 1.;
    bool m_show_overlay = false;
    bool m_show_text = false;
    bool m_wheel_zoom = true;
};
