#include "mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>

#include "tiffprocessapi.h"
#include "utils.h"
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  tiffProcessAPI::getInstance().loadPsTemplate();
  QWidget *central = new QWidget(this);
  this->setCentralWidget(central);
  this->resize(1000, 600);
  QHBoxLayout *hLayout = new QHBoxLayout(central);

  imageView = new ImageView();
  ControlPanel *controlPanel = new ControlPanel();

  hLayout->addWidget(imageView);
  hLayout->addWidget(controlPanel);
  // 用 QSplitter 管理左右控件
  QSplitter *splitter = new QSplitter(Qt::Horizontal); // 水平分割
  splitter->addWidget(imageView);
  splitter->addWidget(controlPanel);
  splitter->setSizes({700, 300}); // 左 200 px，右 400 px
  splitter->setStyleSheet("QSplitter::handle { "
                          "    background-color: #448aff; " // 蓝色
                          "    border-radius: 1px; "
                          "}");
  setCentralWidget(splitter);

  connect(controlPanel, &ControlPanel::openImageClicked, this, [=]() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "打开图像", "", "Images (*.png *.jpg *.bmp *.tif)");
    if (!fileName.isEmpty()) {
      QByteArray path = fileName.toLocal8Bit();
      const char *cpath = path.constData();
      if (getFileType(fileName) == "TIFF") {
        cv::Mat openimage = tiffProcessAPI::getInstance().openTiffImage(
            std::string_view(cpath));
        QPixmap pix = cvMatToQPixmap(openimage);
        imageView->setImage(pix);
        tiffProcessAPI::getInstance().setcvMatImage(openimage);
        return;
      }
      QPixmap pix(fileName);
      imageView->setImage(pix); // 左侧显示图像
      cv::Mat input = cv::imread(cpath, cv::IMREAD_UNCHANGED);
      tiffProcessAPI::getInstance().setcvMatImage(input); // 左侧显示图像
    }
  });

  connect(controlPanel, &ControlPanel::removeBlackFinished, this, [=]() {
    cv::Mat res = tiffProcessAPI::getInstance().geRemoveResult();
    setShowImage(res);
  });

  connect(controlPanel, &ControlPanel::showWhiteClicked, this, [=]() {
    cv::Mat res = tiffProcessAPI::getInstance().getWhite();
    setShowImage(res);
  });
}

int MainWindow::setShowImage(cv::Mat &mats) {
  if (mats.empty()) {
    QMessageBox::information(this, "提示", "无图");
    return -1;
  }

  QPixmap pix = cvMatToQPixmap(mats);
  imageView->setImage(pix);
  return 0;
}
