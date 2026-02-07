#include "controlpanel.h"

#include <qdir.h>

#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>

#include "tiffprocessapi.h"
#include "utils.h"
// 构造函数
ControlPanel::ControlPanel(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);

  QPushButton *openBtn = new QPushButton("打开图像");

  // 下拉框，放在第二个按钮上方
  QComboBox *methodCombo = new QComboBox();
  methodCombo->addItem("GRAY");
  methodCombo->addItem("DARK_NEUTRAL");
  methodCombo->addItem("MAX_CHANNEL");
  methodCombo->setCurrentIndex(2);  // 默认选中第一个

  QPushButton *calcBlackness = new QPushButton("计算黑度");

  QLabel *blacknessLabel = new QLabel("去黑强度");
  blacknessLabel->setAlignment(Qt::AlignHCenter);

  slider = new QSlider(Qt::Horizontal);
  slider->setRange(235, 255);
  slider->setValue(235);        // 默认值
  slider->setSingleStep(1);     // 键盘/微调步长
  slider->setPageStep(10);      // PageUp/PageDown
  slider->setTickInterval(25);  // 刻度间隔
  slider->setTickPosition(QSlider::TicksBelow);

  QLabel *noiseLabel = new QLabel("杂点大小");
  noiseLabel->setAlignment(Qt::AlignHCenter);
  clearSmallslider = new QSlider(Qt::Horizontal);
  clearSmallslider->setRange(1, 100);
  clearSmallslider->setValue(1);  // 默认值
  QPushButton *clearSmall = new QPushButton("去除杂点");
  QPushButton *showWhite = new QPushButton("显示白色");
  QPushButton *generateNew = new QPushButton("生成tiff");
  QPushButton *testbtn = new QPushButton("test");

  // 添加控件到布局
  layout->addWidget(openBtn);
  layout->addWidget(methodCombo);  // 下拉框在按钮上方
  layout->addWidget(calcBlackness);
  layout->addWidget(blacknessLabel);
  layout->addWidget(slider);
  layout->addWidget(noiseLabel);
  layout->addWidget(clearSmallslider);
  layout->addWidget(clearSmall);
  // layout->addWidget(showWhite);
  layout->addWidget(generateNew);
  layout->addWidget(testbtn);

  layout->addStretch();  // 控件靠上

  // 信号连接
  connect(openBtn, &QPushButton::clicked, this,
          &ControlPanel::openImageClicked);
  connect(calcBlackness, &QPushButton::clicked, this, [=]() {
    int methodIndex = methodCombo->currentIndex();
    BlacknessMethod method = static_cast<BlacknessMethod>(methodIndex);
    int res = tiffProcessAPI::getInstance().calBackness(method);
    if (res != 0) return;
    QMessageBox::information(this, tr("提示"), tr("黑度计算完成"));
  });

  connect(slider, &QSlider::valueChanged, this, [=](int value) {
    if (0 != tiffProcessAPI::getInstance().removeBlack(value)) return;
    emit removeBlackFinished();
  });

  connect(clearSmall, &QPushButton::clicked, this, [=]() {
    int kernel = clearSmallslider->value();
    int res = tiffProcessAPI::getInstance().removeSmallByArea(kernel);
    if (res != 0) return;
    emit removeBlackFinished();
  });
  connect(showWhite, &QPushButton::clicked, this, [=]() {
    int res = tiffProcessAPI::getInstance().generateWhiteCompensation(
        slider->value());
    if (0 != res) return;
    emit showWhiteClicked();
  });
  connect(generateNew, &QPushButton::clicked, this, [=]() {
    QString filePath = QFileDialog::getSaveFileName(
        this, tr("保存 TIFF 文件"), QDir::homePath(),
        tr("TIFF 文件 (*.tif *.tiff)"));
    if (filePath.isEmpty()) {
      return;  // 用户取消
    }
    // 自动补 .tif 后缀
    if (!filePath.endsWith(".tif", Qt::CaseInsensitive) &&
        !filePath.endsWith(".tiff", Qt::CaseInsensitive)) {
      filePath += ".tif";
    }
    int methodIndex = methodCombo->currentIndex();
    BlacknessMethod method = static_cast<BlacknessMethod>(methodIndex);

    int res = tiffProcessAPI::getInstance().genernateTiffFile(
        std::string_view(filePath.toStdString()), method, this->slider->value(),
        this->clearSmallslider->value());
    if (0 != res) return;
    QMessageBox::information(this, tr("提示"), tr("保存成功"));
  });
  connect(testbtn, &QPushButton::clicked, this,
          [=]() { return tiffProcessAPI::getInstance().test(); });
}
