#ifndef UTILS_H
#define UTILS_H
#include <qfileinfo.h>
#include <qimage.h>
#include <qpixmap.h>

#include <QDebug>
#include <opencv2/opencv.hpp>

#define DEBUG qDebug().noquote() << "[" << __FILE__ << ":" << __LINE__ << "]"

inline QString getFileType(const QString& filename) {
  QString ext = QFileInfo(filename).suffix().toLower();
  if (ext == "png") return "PNG";
  if (ext == "jpg" || ext == "jpeg") return "JPG";
  if (ext == "tif" || ext == "tiff") return "TIFF";
  return "Unknown";
}

inline QPixmap cvMatToQPixmap(const cv::Mat& mat) {
  if (mat.empty()) return QPixmap();

  QImage img;

  switch (mat.type()) {
    case CV_8UC1:  // 单通道灰度图
      img = QImage(mat.data, mat.cols, mat.rows, mat.step,
                   QImage::Format_Grayscale8);
      break;
    case CV_8UC3:  // 3通道 BGR
      img =
          QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
      cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);  // 转为 RGB
      break;
    case CV_8UC4:  // 4通道 BGRA
      img = QImage(mat.data, mat.cols, mat.rows, mat.step,
                   QImage::Format_RGBA8888);
      cv::cvtColor(mat, mat, cv::COLOR_BGRA2RGBA);
      break;
    default:
      // 不支持类型
      return QPixmap();
  }

  return QPixmap::fromImage(img.copy());  // copy 保证数据独立
}
#endif  // UTILS_H
