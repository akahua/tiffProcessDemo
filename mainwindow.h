#include <QHBoxLayout>
#include <QMainWindow>
#include <QWidget>

#include "ControlPanel.h"
#include "ImageView.h"

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  MainWindow(QWidget *parent = nullptr);

  int setShowImage(cv::Mat &mats);

 private:
  ImageView *imageView;
};
