#ifndef TIFFPROCESSAPI_H
#define TIFFPROCESSAPI_H
#include "pstemplate.h"
#include "tiffprocess.h"
class tiffProcessAPI {
public:
  // 获取单例实例
  static tiffProcessAPI &getInstance();

public:
  cv::Mat openTiffImage(std::string_view path);

  void setcvMatImage(const cv::Mat mat);

  int calBackness(BlacknessMethod type = BlacknessMethod::GRAY);

  int removeBlack(int thresh);

  int removeSmall(int kernelSize);

  int removeSmallByArea(int thresh);

  int generateWhiteCompensation(int thresh);

  int genernateTiffFile(std::string_view path, BlacknessMethod type,
                        int blacknessThresh, int noiseThresh);

  int test();

  int loadPsTemplate();

public:
  cv::Mat geRemoveResult();
  cv::Mat geRemoveSmallResult();
  cv::Mat getProcessTransparent();
  cv::Mat getTransparent();
  cv::Mat getWhite();

private:
  tiffProcessAPI() = default;
  ~tiffProcessAPI() = default;

  tiffProcessAPI(const tiffProcessAPI &) = delete;
  tiffProcessAPI &operator=(const tiffProcessAPI &) = delete;

  tiffProcessAPI(tiffProcessAPI &&) = delete;
  tiffProcessAPI &operator=(tiffProcessAPI &&) = delete;

protected:
  PsTemplate _ps;
  cv::Mat _origin;
  std::vector<cv::Mat> _orgins;
  cv::Mat _transparent;
  cv::Mat _processTransparent;
  cv::Mat _white;
  cv::Mat _removeShowMat;
  cv::Mat _blackness;
};

#endif // TIFFPROCESSAPI_H
