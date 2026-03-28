#pragma once
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QMutex>
#include <QPushButton>
#include <QRunnable>
#include <QTimer>
#include <QWidget>

#include "ImageItem.h"
#include "ImageLoaderQueue.h"

inline constexpr int fitincircularrange(int i, int size) {
  int r = i % size;
  return (r < 0) ? r + size : r;
}

class ImgView : public QWidget {
  Q_OBJECT

  enum class FileDir { none, next, previous };

public:
  ImgView(QWidget *parent = 0);
  ~ImgView();

  void autofit();
  void loadImage(QStringList filename);
  void openFolder(QString dir);
  void loaded(WorkItem info);
  void loadedFilenames(QList<WorkItem> is);
  void loadedImage(WorkItem wi, QImage img, QImage thumb, QSize si);

  protected:
  void paintEvent(QPaintEvent *) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *) override;
  void resizeEvent(QResizeEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

signals:
  void message(QString);

private:
  void customContextMenu(QPoint pos);
  void getFiles(QStringList filenames, QDirIterator::IteratorFlag itf);
  void btnWheelZoomIconUpdate();
  void nextImage(FileDir fd);
  void setTitle();
  void clearImages();
  void setTransform();
  void openDatabase();

  QList<ImageItem *> m_allImages;
  ImageItem *m_mainImage = nullptr;
  ImageLoaderQueue m_imageloaderqueue;
  QSizeF m_visibleImage_size;
  QMutex m_allImage_mutex;
  QVector<QPushButton *> m_buttons;
  QPointF m_lastMousePos;
  QPointF m_lastMouseHoverPos;
  QPointF m_mouselogicalpos;
  QPointF m_offset;
  QPoint m_pressed;
  double m_zoom = 1.;
  QTransform m_transform;
  bool m_show_overlay = false;
  bool m_show_text = false;
  bool m_wheel_zoom = true;
  bool m_show_thumb = false;
  bool m_antialiase = false;
  int m_thumbcount = 0;
};
