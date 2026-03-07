#pragma once
#include <QFileInfo>
#include <QString>

struct ImageInfo {
  enum class WorkType { none, loadImage, loadThumbnail256, destroyImage };
  QFileInfo fi;
  WorkType worktype = WorkType::none;
  QString m_error_message;
  void *m_imageitem = nullptr;
};