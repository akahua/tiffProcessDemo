#include "tiffprocessapi.h"

#include "utils.h"

tiffProcessAPI &tiffProcessAPI::getInstance() {
  static tiffProcessAPI instance;
  return instance;
}

void tiffProcessAPI::setcvMatImage(const cv::Mat mat) { this->_origin = mat; }

int tiffProcessAPI::calBackness(BlacknessMethod type) {
  int res = tiffProcess::getInstance().calcBlackness(_origin, type, _blackness);
  if (res != 0)
    return res;
  return 0;
}

int tiffProcessAPI::removeBlack(int thresh) {
  _transparent = cv::Mat(this->_origin.size(), CV_8UC1);
  int res =
      tiffProcess::getInstance().removeBlack(_blackness, thresh, _transparent);
  if (res != 0)
    return res;
  std::vector<cv::Mat> merge = this->_orgins;
  merge.push_back(_transparent);
  cv::merge(merge, _removeShowMat);
  return 0;
}

int tiffProcessAPI::removeSmall(int kernelSize) {
  int size = kernelSize * 2 - 1;

  cv::Mat kernel =
      cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(size, size));

  cv::morphologyEx(_transparent, _processTransparent, cv::MORPH_CLOSE, kernel);

  std::vector<cv::Mat> merge = this->_orgins;
  merge.push_back(_processTransparent);
  cv::merge(merge, _removeShowMat);
  return 0;
}

int tiffProcessAPI::removeSmallByArea(int thresh) {
  int res = tiffProcess::getInstance().removeSmallComponents(
      _transparent, thresh, _processTransparent);
  if (res != 0)
    return res;
  std::vector<cv::Mat> merge = this->_orgins;
  merge.push_back(_processTransparent);
  cv::merge(merge, _removeShowMat);

  return 0;
}

int tiffProcessAPI::generateWhiteCompensation(int thresh) {
  tiffProcess::getInstance().generateWhiteCompensation(
      _blackness, _processTransparent, thresh, _white);
  return 0;
}

int tiffProcessAPI::genernateTiffFile(std::string_view path,
                                      BlacknessMethod type, int blacknessThresh,
                                      int noiseThresh) {
  return tiffProcess::getInstance().genernateTiffFile(
      path, type, blacknessThresh, noiseThresh, this->_ps);
}

inline void drawRotatedRect(cv::Mat &img, const cv::RotatedRect &rect,
                            const cv::Scalar &color = cv::Scalar(0, 255, 0),
                            int thickness = 2) {
  cv::Point2f vertices[4];
  rect.points(vertices); // 获取矩形四个顶点

  for (int i = 0; i < 4; i++)
    cv::line(img, vertices[i], vertices[(i + 1) % 4], color, thickness);
}

int computeMinAreaRect(cv::Mat &input, cv::RotatedRect &rotRect,
                       int binaryType = 0) {
  if (input.empty())
    return -1;

  cv::Mat bin;
  cv::threshold(input, bin, 0, 255, binaryType | cv::THRESH_OTSU);

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(bin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  if (contours.empty())
    return -1;

  std::vector<cv::Point> allPts;
  for (auto &c : contours) {
    if (cv::contourArea(c) < 30)
      continue;
    allPts.insert(allPts.end(), c.begin(), c.end());
  }
  if (allPts.size() < 5)
    return -1;

  rotRect = cv::minAreaRect(allPts);

  return 0;
}

int tiffProcessAPI::test() {
  cv::Mat input = cv::imread("D:/data/32tiff/2411181631034_clip_.png",
                             cv::IMREAD_GRAYSCALE);
  cv::RotatedRect rt;
  computeMinAreaRect(input, rt, 1);
  drawRotatedRect(input, rt);
  cv::imshow("", input);
  cv::waitKey();

  return 0;
}

int tiffProcessAPI::loadPsTemplate() {
  this->_ps.load("withW.tif");
  DEBUG << "load template tiff successfully";
  return 0;
}

cv::Mat tiffProcessAPI::geRemoveResult() { return this->_removeShowMat; }

cv::Mat tiffProcessAPI::getProcessTransparent() {
  return this->_processTransparent;
}

cv::Mat tiffProcessAPI::getTransparent() { return this->_transparent; }

cv::Mat tiffProcessAPI::getWhite() { return this->_white; }

cv::Mat tiffProcessAPI::openTiffImage(std::string_view path) {
  cv::Mat out;
  tiffProcess::getInstance().loadTiff(path, out);
  _orgins.clear();
  cv::split(out, _orgins);
  return out;
}
