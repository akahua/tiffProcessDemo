#include "tiffprocess.h"

#include "utils.h"
#include <cstdio>

#define TIFF_DBG(fmt, ...)                                                     \
  do {                                                                         \
    printf("[TIFF] " fmt "\n", ##__VA_ARGS__);                                 \
    fflush(stdout);                                                            \
  } while (0)
static inline uint8_t clamp8(int v) {
  return static_cast<uint8_t>(std::max(0, std::min(255, v)));
}

tiffProcess &tiffProcess::getInstance() {
  static tiffProcess instance; // 局部静态变量，C++11 保证线程安全
  return instance;
}

int tiffProcess::readTiffImage(std::string_view path, TiffImage &image) {
  TIFF *tif = TIFFOpen(std::string(path).c_str(), "r");
  if (!tif) {
    return -1; // 打开失败
  }

  TiffMeta &meta = image.meta;
  TiffRawData &raw = image.raw;
  float xres = 0.0f, yres = 0.0f;
  uint16_t resUnit = RESUNIT_INCH;
  // ---------------- 基本 Tag ----------------
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &meta.width);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &meta.height);
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &meta.samplesPerPixel);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &meta.bitsPerSample);
  TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &meta.photometric);
  TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &meta.planarConfig);
  TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &meta.orientation);
  TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &meta.compression);
  TIFFGetField(tif, TIFFTAG_XRESOLUTION, &xres);
  TIFFGetField(tif, TIFFTAG_YRESOLUTION, &yres);
  TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resUnit);

  image.meta.xResolution = xres;
  image.meta.yResolution = yres;
  image.meta.resolutionUnit = resUnit;
  // ---------------- ExtraSamples ----------------
  uint16_t extraCount = 0;
  uint16_t *extraInfo = nullptr;
  if (TIFFGetField(tif, TIFFTAG_EXTRASAMPLES, &extraCount, &extraInfo)) {
    meta.extraSamples.assign(extraInfo, extraInfo + extraCount);
  } else {
    meta.extraSamples.clear();
  }

  // ---------------- 校验 ----------------
  if (meta.bitsPerSample != 8) {
    TIFFClose(tif);
    return -2; // 当前实现只支持 8bit
  }

  if (meta.samplesPerPixel == 0 || meta.width == 0 || meta.height == 0) {
    TIFFClose(tif);
    return -3; // 非法 TIFF
  }

  // ---------------- 读取像素数据 ----------------
  const tsize_t scanlineSize = TIFFScanlineSize(tif);
  raw.bytesPerRow = static_cast<uint32_t>(scanlineSize);

  raw.buffer.resize(static_cast<size_t>(scanlineSize) * meta.height);

  if (meta.planarConfig == PLANARCONFIG_CONTIG) {
    // 通道交错（最常见）
    for (uint32_t y = 0; y < meta.height; ++y) {
      uint8_t *dst = raw.buffer.data() + y * scanlineSize;
      if (TIFFReadScanline(tif, dst, y) < 0) {
        TIFFClose(tif);
        return -4;
      }
    }
  } else {
    // PLANARCONFIG_SEPARATE（每个通道一个 plane）
    const uint32_t planes = meta.samplesPerPixel;
    const size_t planeSize = static_cast<size_t>(scanlineSize) * meta.height;

    raw.buffer.resize(planeSize * planes);

    for (uint32_t p = 0; p < planes; ++p) {
      for (uint32_t y = 0; y < meta.height; ++y) {
        uint8_t *dst = raw.buffer.data() + p * planeSize + y * scanlineSize;

        if (TIFFReadScanline(tif, dst, y, p) < 0) {
          TIFFClose(tif);
          return -5;
        }
      }
    }
  }

  TIFFClose(tif);
  return 0;
}

int tiffProcess::generateRgbMat(const TiffImage &image, cv::Mat &outRgb) {
  const auto &meta = image.meta;
  const auto &buf = image.raw.buffer;

  if (meta.bitsPerSample != 8) {
    return -1; // 只支持 8-bit
  }

  if (meta.planarConfig != PLANARCONFIG_CONTIG) {
    return -2; // 暂不支持 planar
  }

  const uint32_t width = meta.width;
  const uint32_t height = meta.height;
  const uint16_t spp = meta.samplesPerPixel;

  if (width == 0 || height == 0 || spp < 3) {
    return -3;
  }

  outRgb.create(height, width, CV_8UC3);

  const uint8_t *src = buf.data();
  uint8_t *dst = outRgb.data;

  const size_t pixelStride = spp;

  if (meta.photometric == PHOTOMETRIC_RGB) {
    // -------- RGB → RGB --------
    for (uint32_t y = 0; y < height; ++y) {
      for (uint32_t x = 0; x < width; ++x) {
        const uint8_t *p = src + (y * width + x) * pixelStride;

        // TIFF: RGB
        uint8_t R = p[0];
        uint8_t G = p[1];
        uint8_t B = p[2];

        // OpenCV: BGR
        *dst++ = B;
        *dst++ = G;
        *dst++ = R;
      }
    }
  } else if (meta.photometric == PHOTOMETRIC_SEPARATED) {
    // -------- CMYK → RGB --------
    if (spp < 4)
      return -4;

    for (uint32_t y = 0; y < height; ++y) {
      for (uint32_t x = 0; x < width; ++x) {
        const uint8_t *p = src + (y * width + x) * pixelStride;

        int C = p[0];
        int M = p[1];
        int Y = p[2];
        int K = p[3];

        // 工业常见 CMYK → RGB（非 ICC）
        uint8_t R = clamp8(255 - (C + K));
        uint8_t G = clamp8(255 - (M + K));
        uint8_t B = clamp8(255 - (Y + K));

        *dst++ = B;
        *dst++ = G;
        *dst++ = R;
      }
    }
  } else {
    return -5; // 不支持的 Photometric
  }

  return 0;
}

int tiffProcess::updateExtraChannels(TiffImage &image, const cv::Mat &alpha,
                                     const cv::Mat &extra1,
                                     const cv::Mat &extra2) {
  TiffMeta &meta = image.meta;
  TiffRawData &raw = image.raw;

  // ---------------- 基本校验 ----------------
  if (meta.bitsPerSample != 8 || meta.planarConfig != PLANARCONFIG_CONTIG) {
    return -1;
  }

  if (alpha.empty() || extra1.empty() || extra2.empty()) {
    return -2;
  }

  if (alpha.type() != CV_8UC1 || extra1.type() != CV_8UC1 ||
      extra2.type() != CV_8UC1) {
    return -3;
  }

  if (alpha.cols != static_cast<int>(meta.width) ||
      alpha.rows != static_cast<int>(meta.height) ||
      extra1.size() != alpha.size() || extra2.size() != alpha.size()) {
    return -4;
  }

  // ---------------- 颜色通道数 ----------------
  int colorChannels = -1;
  if (meta.photometric == PHOTOMETRIC_RGB) {
    colorChannels = 3;
  } else if (meta.photometric == PHOTOMETRIC_SEPARATED) {
    colorChannels = 4;
  } else {
    return -5;
  }

  // ---------------- Alpha 判断 ----------------
  int alphaExtraIdx = findAlphaExtraIndex(meta);
  bool hasAlpha = (alphaExtraIdx >= 0);

  // ---------------- 构造新的 ExtraSamples ----------------
  std::vector<uint16_t> newExtraSamples;

  if (hasAlpha) {
    // 保留原顺序（包含 Alpha）
    newExtraSamples = meta.extraSamples;
  } else {
    // 插入 Alpha 到最前
    newExtraSamples.push_back(EXTRASAMPLE_UNASSALPHA);
    for (uint16_t t : meta.extraSamples) {
      newExtraSamples.push_back(t);
    }
  }

  // 追加两个新通道
  newExtraSamples.push_back(EXTRASAMPLE_UNSPECIFIED);
  newExtraSamples.push_back(EXTRASAMPLE_UNSPECIFIED);

  // ---------------- sample 计算 ----------------
  const int oldSpp = meta.samplesPerPixel;
  const int newExtraCount = static_cast<int>(newExtraSamples.size());
  const int newSpp = colorChannels + newExtraCount;

  // 旧 Alpha 在 sample 中的位置（若存在）
  int oldAlphaSample = -1;
  if (hasAlpha) {
    oldAlphaSample = colorChannels + alphaExtraIdx;
  }

  // 新 Alpha 在 sample 中的位置（永远是第一个 Extra）
  const int newAlphaSample = colorChannels;

  // ---------------- 重建 Raw Buffer ----------------
  const size_t pixelCount = static_cast<size_t>(meta.width) * meta.height;

  std::vector<uint8_t> newBuffer(pixelCount * newSpp);

  const uint8_t *src = raw.buffer.data();
  uint8_t *dst = newBuffer.data();

  for (size_t i = 0; i < pixelCount; ++i) {
    // 1. 颜色通道
    memcpy(dst, src, colorChannels);

    // 2. Alpha（覆盖或新建）
    dst[newAlphaSample] = alpha.data[i];

    // 3. 拷贝旧 Extra（跳过旧 Alpha）
    int dstIdx = colorChannels + 1;
    int srcIdx = colorChannels;

    for (size_t e = 0; e < meta.extraSamples.size(); ++e) {
      if (hasAlpha && static_cast<int>(e) == alphaExtraIdx) {
        srcIdx++; // 跳过旧 Alpha
        continue;
      }
      dst[dstIdx++] = src[srcIdx++];
    }

    // 4. 新增两个通道
    dst[dstIdx++] = extra1.data[i];
    dst[dstIdx++] = extra2.data[i];

    src += oldSpp;
    dst += newSpp;
  }

  // ---------------- 更新 meta / raw ----------------
  meta.extraSamples = std::move(newExtraSamples);
  meta.samplesPerPixel = static_cast<uint16_t>(newSpp);
  raw.buffer = std::move(newBuffer);

  return 0;
}

int tiffProcess::loadTiff(std::string_view path, cv::Mat &outRgb) {
  int res = readTiffImage(path, this->_tiff);
  if (res != 0) {
    return res;
  }

  dumpTiffImageInfo(this->_tiff);
  auto p = this->_tiff.raw.buffer.data();
  res = generateRgbMat(this->_tiff, outRgb);
  return res;
}

int tiffProcess::writeTiff(std::string_view path, const TiffImage &image,
                           const PsTemplate &ps) {

  const TiffMeta &meta = image.meta;
  const TiffRawData &raw = image.raw;

  if (raw.buffer.empty())
    return -1;

  TIFF *tif = TIFFOpen(path.data(), "w");

  if (!tif)
    return -2;

  // ---- Basic ----
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, meta.width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, meta.height);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, meta.samplesPerPixel);
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, meta.photometric);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, meta.planarConfig);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, meta.orientation);

  // BitsPerSample (uniform)
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t)meta.bitsPerSample);

  // Resolution
  TIFFSetField(tif, TIFFTAG_XRESOLUTION, meta.xResolution);
  TIFFSetField(tif, TIFFTAG_YRESOLUTION, meta.yResolution);
  TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, meta.resolutionUnit);

  // Compression
  TIFFSetField(tif, TIFFTAG_COMPRESSION, meta.compression);

  // Strips
  TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0));

  // ExtraSamples
  if (!meta.extraSamples.empty()) {
    TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, (uint16_t)meta.extraSamples.size(),
                 meta.extraSamples.data());
  }

  // Photoshop template (Spot)
  if (!ps.ps34377.empty()) {

    // safety check
    if (meta.samplesPerPixel != ps.spp ||
        meta.extraSamples.size() != ps.extra) {
      TIFFClose(tif);
      return -10; // template mismatch
    }

    TIFFSetField(tif, TIFFTAG_PHOTOSHOP, (uint32)ps.ps34377.size(),
                 ps.ps34377.data());
  }
  const tsize_t sl = TIFFScanlineSize(tif);
  const size_t expectedRow =
      (size_t)meta.width * meta.samplesPerPixel * (meta.bitsPerSample / 8);

  printf("[WRT] TIFFScanlineSize=%lld expectedRow=%zu\n", (long long)sl,
         expectedRow);
  fflush(stdout);

  // ---- Write pixels ----
  const tsize_t lineBytes = TIFFScanlineSize(tif);
  const uint8_t *buffer = raw.buffer.data();

  for (uint32_t row = 0; row < meta.height; ++row) {
    if (TIFFWriteScanline(tif, (void *)(buffer + row * lineBytes), row, 0) <
        0) {
      TIFFClose(tif);
      return -3;
    }
  }

  TIFFClose(tif);
  return 0;
}

int tiffProcess::calcBlackness(const cv::Mat &rgb, BlacknessMethod method,
                               cv::Mat &blackness) {
  if (rgb.empty() || rgb.type() != CV_8UC3)
    return -1;

  blackness.create(rgb.size(), CV_8UC1);

  for (int y = 0; y < rgb.rows; ++y) {
    const cv::Vec3b *src = rgb.ptr<cv::Vec3b>(y);
    uchar *dst = blackness.ptr<uchar>(y);

    for (int x = 0; x < rgb.cols; ++x) {
      uchar R = src[x][0];
      uchar G = src[x][1];
      uchar B = src[x][2];

      uchar value = 0;

      switch (method) {
      case BlacknessMethod::GRAY: {
        //
        value = static_cast<uchar>(0.299f * R + 0.587f * G + 0.114f * B);
        break;
      }
      case BlacknessMethod::DARK_NEUTRAL: {
        //暗度
        float brightness = (R + G + B) / (3.0f * 255.0f);
        float dark = 1.0f - brightness;

        //中性色
        uchar maxv = std::max({R, G, B});
        uchar minv = std::min({R, G, B});
        float chroma = (maxv - minv) / 255.0f;
        float neutral = 1.0f - chroma;

        float b = dark * neutral;
        value = static_cast<uchar>(std::clamp(b, 0.0f, 1.0f) * 255.0f);
        break;
      }
      case BlacknessMethod::MAX_CHANNEL: {
        // 近似 K = 1 - max(R,G,B)
        uchar maxv = std::max({R, G, B});
        value = 255 - maxv;
        break;
      }
      default:
        return -2;
      }

      dst[x] = value;
    }
  }

  return 0;
}

int tiffProcess::removeBlack(const cv::Mat &blackness, int thresh,
                             cv::Mat &output) {
  if (blackness.empty() || blackness.type() != CV_8UC1)
    return -1;
  cv::threshold(blackness, output, thresh, 255, cv::THRESH_BINARY_INV);
  return 0;
}

int tiffProcess::removeSmallComponents(const cv::Mat &input, int minArea,
                                       cv::Mat &output) {
  if (input.empty() || input.type() != CV_8UC1)
    return -1;

  cv::Mat labels, stats, centroids;

  cv::connectedComponentsWithStats(input, labels, stats, centroids,
                                   8, // 8连通
                                   CV_32S);

  output = cv::Mat::zeros(labels.size(), CV_8UC1);

  for (int y = 0; y < labels.rows; ++y) {
    const int *labelRow = labels.ptr<int>(y);
    uchar *outRow = output.ptr<uchar>(y);

    for (int x = 0; x < labels.cols; ++x) {
      int lbl = labelRow[x];
      if (lbl > 0 && stats.at<int>(lbl, cv::CC_STAT_AREA) >= minArea) {
        outRow[x] = 255;
      }
    }
  }
  return 0;
}

int tiffProcess::generateWhiteCompensation(const cv::Mat &blackness,
                                           const cv::Mat &transparent,
                                           int thresh, cv::Mat &white) {
  // -------- 参数检查 --------
  if (blackness.empty() || transparent.empty())
    return -1;

  if (blackness.type() != CV_8UC1 || transparent.type() != CV_8UC1)
    return -2;

  if (blackness.size() != transparent.size())
    return -3;

  if (thresh <= 0 || thresh > 255)
    return -4;

  // -------- 输出初始化 --------
  white.create(blackness.size(), CV_8UC1);
  white.setTo(0);

  // -------- 主循环 --------
  for (int y = 0; y < blackness.rows; ++y) {
    const uchar *bptr = blackness.ptr<uchar>(y);
    const uchar *tptr = transparent.ptr<uchar>(y);
    uchar *wptr = white.ptr<uchar>(y);

    for (int x = 0; x < blackness.cols; ++x) {
      // 1. 透明像素：不补白
      if (tptr[x] == 0) {
        wptr[x] = 0;
        continue;
      }

      int b = bptr[x];

      // 2. 黑度高于阈值：不补白
      if (b >= thresh) {
        wptr[x] = 0;
        continue;
      }

      // 3. 连续白色补偿
      int val = (thresh - b) * 255 / thresh;
      wptr[x] = static_cast<uchar>(std::clamp(val, 0, 255));
    }
  }
  return 0;
}

int tiffProcess::genernateTiffFile(std::string_view path,
                                   BlacknessMethod method, int blacknessThresh,
                                   int noiseThresh, const PsTemplate &ps) {
  cv::Mat rgbImg;
  int res;

  res = generateRgbMat(this->_tiff, rgbImg);
  if (res != 0)
    return res;
  cv::Mat blackness;
  res = calcBlackness(rgbImg, method, blackness);
  if (res != 0)
    return res;

  if (rgbImg.empty())
    return -1;
  cv::Mat noBlack;
  res = removeBlack(blackness, blacknessThresh, noBlack);
  if (res != 0)
    return res;
  cv::Mat noNoise;
  res = removeSmallComponents(noBlack, noiseThresh, noNoise);
  if (res != 0)
    return res;
  cv::Mat whiteCompensation;
  res = generateWhiteCompensation(blackness, noNoise, blacknessThresh,
                                  whiteCompensation);
  if (res != 0)
    return res;
  cv::Mat whiteInk = 255 - whiteCompensation;
  res = updateExtraChannels(this->_tiff, noNoise, whiteInk, whiteInk.clone());
  if (res != 0)
    return res;

  res = writeTiff(path, this->_tiff, ps);
  if (res != 0)
    return res;
  return 0;
}
