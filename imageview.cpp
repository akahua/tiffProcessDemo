#include "imageview.h"

#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>
ImageView::ImageView(QWidget *parent) : QGraphicsView(parent) {
  scene = new QGraphicsScene(this);
  this->setScene(scene);
  this->setFocusPolicy(Qt::StrongFocus);  // 接收焦点
  this->setFocus();                       // 设置为当前焦点
}

void ImageView::setImage(const QPixmap &pix) {
  scene->clear();
  pixmapItem = scene->addPixmap(pix);
  this->fitInView(pixmapItem, Qt::KeepAspectRatio);
}

void ImageView::wheelEvent(QWheelEvent *event) {
  if (!pixmapItem) return;

  // 判断 Ctrl 是否按下
  bool ctrlPressed = event->modifiers() & Qt::ControlModifier;

  // 普通缩放倍数
  double scaleStep = ctrlPressed ? 1.5 : 1.15;  // Ctrl+滚轮 → 放大更快

  if (event->angleDelta().y() > 0) {
    scale(scaleStep, scaleStep);  // 放大
    scaleFactor *= scaleStep;
  } else {
    scale(1.0 / scaleStep, 1.0 / scaleStep);  // 缩小
    scaleFactor /= scaleStep;
  }

  event->accept();  // 阻止事件传递
}

void ImageView::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    lastMousePos = event->pos();
    dragging = true;
    setCursor(Qt::ClosedHandCursor);
  }
  QGraphicsView::mousePressEvent(event);
}

void ImageView::mouseMoveEvent(QMouseEvent *event) {
  if (dragging) {
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
  }
  QGraphicsView::mouseMoveEvent(event);
}

void ImageView::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    dragging = false;
    setCursor(Qt::ArrowCursor);
  }
  QGraphicsView::mouseReleaseEvent(event);
}

void ImageView::keyPressEvent(QKeyEvent *event) {
  // Ctrl + H
  if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_H) {
    if (!pixmapItem) return;  // 没有图像直接返回

    QRectF rect = pixmapItem->sceneBoundingRect();
    if (!rect.isValid() || rect.isEmpty()) return;  // 避免空矩形

    // 重置平移和缩放
    this->resetTransform();

    // 让图像适应视图大小并居中
    this->fitInView(rect, Qt::KeepAspectRatio);
  } else {
    QGraphicsView::keyPressEvent(event);
  }
}
