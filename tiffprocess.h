#ifndef TIFFPROCESS_H
#define TIFFPROCESS_H
#include <opencv2/opencv.hpp>

#include "pstemplate.h"
#include "tiffimage.h"
enum class BlacknessMethod {
  GRAY = 0,     // 纯灰度（调试 / 对照用）
  DARK_NEUTRAL, // 暗 + 中性（推荐）
  MAX_CHANNEL   // 近似 K = 1 - max(R,G,B)
};

class tiffProcess {
public:
  static tiffProcess &getInstance();

  //加载tiff
  int loadTiff(std::string_view path, cv::Mat &outRgb);

  int calcBlackness(const cv::Mat &rgb, BlacknessMethod method,
                    cv::Mat &blackness);

  int removeBlack(const cv::Mat &blackness, int thresh, cv::Mat &output);

  int removeSmallComponents(const cv::Mat &input, int minArea, cv::Mat &output);

  int generateWhiteCompensation(const cv::Mat &blackness,
                                const cv::Mat &transparent, int thresh,
                                cv::Mat &white);

  int genernateTiffFile(std::string_view path, BlacknessMethod method,
                        int blacknessThresh, int noiseThresh,
                        const PsTemplate &ps);

private:
  int readTiffImage(std::string_view path, TiffImage &image);

  int writeTiff(std::string_view path, const TiffImage &image,
                const PsTemplate &ps);

  int generateRgbMat(const TiffImage &image, cv::Mat &outRgb);

  int updateExtraChannels(TiffImage &image, const cv::Mat &alpha,
                          const cv::Mat &extra1, const cv::Mat &extra2);

protected:
  TiffImage _tiff;

private:
  tiffProcess() = default;
  ~tiffProcess() = default;

  tiffProcess(const tiffProcess &) = delete;
  tiffProcess &operator=(const tiffProcess &) = delete;

  tiffProcess(tiffProcess &&) = delete;
  tiffProcess &operator=(tiffProcess &&) = delete;
};

#endif // TIFFPROCESS_H
