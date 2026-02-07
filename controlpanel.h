// ControlPanel.h
#pragma once
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

class ControlPanel : public QWidget {
  Q_OBJECT
 public:
  ControlPanel(QWidget *parent = nullptr);

 signals:
  void openImageClicked();

  void calcBalcknessClicked();

  void removeBlackFinished();

  void showWhiteClicked();

 private:
  QSlider *slider;

  QSlider *clearSmallslider;
};
