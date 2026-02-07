#include <QApplication>
#include <opencv2/opencv.hpp>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
  qputenv("QT_IMAGEIO_MAXALLOC",
          QByteArray::number(1024 * 1024 * 1024));  // 1GB

  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}
