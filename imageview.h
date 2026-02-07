// ImageView.h
#pragma once
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <opencv2/opencv.hpp>
class ImageView : public QGraphicsView {
  Q_OBJECT
 public:
  ImageView(QWidget *parent = nullptr);

  void setImage(const QPixmap &pix);

 protected:
  void wheelEvent(QWheelEvent *event) override;

  void mousePressEvent(QMouseEvent *event) override;

  // 鼠标移动
  void mouseMoveEvent(QMouseEvent *event) override;

  // 鼠标释放
  void mouseReleaseEvent(QMouseEvent *event) override;

  void keyPressEvent(QKeyEvent *event) override;

 private:
  QGraphicsScene *scene;
  QGraphicsPixmapItem *pixmapItem;
  double scaleFactor;

  bool dragging;
  QPoint lastMousePos;
};
