#ifndef TIFFIMAGE_H
#define TIFFIMAGE_H
#include <tiffio.h>

#include <vector>

#include "utils.h"
struct TiffMeta {
  // ---------------- 基本尺寸 ----------------
  uint32_t width = 0;   // 图像宽度（像素）
  uint32_t height = 0;  // 图像高度（像素）

  // ---------------- 像素格式 ----------------
  uint16_t samplesPerPixel = 0;  // 每像素通道数（RGB=3, RGBA=4, CMYK=4+）
  uint16_t bitsPerSample = 0;    // 每通道位深（通常 8 或 16）
  uint16_t photometric = 0;      // 颜色模型（RGB / CMYK / Gray 等）
  uint16_t planarConfig = PLANARCONFIG_CONTIG;  // 通道排列方式（交错 / 分平面）

  float xResolution = 0.0f;                // X 方向 DPI
  float yResolution = 0.0f;                // Y 方向 DPI
  uint16_t resolutionUnit = RESUNIT_INCH;  // 单位：英寸（DPI）
  // ---------------- ExtraSamples ----------------
  // TIFFTAG_EXTRASAMPLES
  // 内容示例：
  //  - EXTRASAMPLE_ASSOCALPHA   → Alpha（预乘）
  //  - EXTRASAMPLE_UNASSALPHA   → Alpha（未预乘）
  //  - EXTRASAMPLE_UNSPECIFIED  → 未指定用途（专色、Mask 等）
  std::vector<uint16_t> extraSamples;

  // ---------------- 常见可选 Tag ----------------
  uint16_t orientation = ORIENTATION_TOPLEFT;  // 图像方向
  uint16_t compression = COMPRESSION_NONE;     // 压缩方式

  // ---------------- 语义推导（不是 Tag） ----------------

  // 是否包含 Alpha 通道
  bool hasAlpha() const {
    for (auto v : extraSamples) {
      if (v == EXTRASAMPLE_ASSOCALPHA || v == EXTRASAMPLE_UNASSALPHA)
        return true;
    }
    return false;
  }

  // Alpha 在 sample 中的索引（不存在返回 -1）
  int alphaSampleIndex() const {
    int base = baseColorSamples();
    for (size_t i = 0; i < extraSamples.size(); ++i) {
      if (extraSamples[i] == EXTRASAMPLE_ASSOCALPHA ||
          extraSamples[i] == EXTRASAMPLE_UNASSALPHA)
        return base + static_cast<int>(i);
    }
    return -1;
  }

  // 基础颜色通道数量（由 photometric 决定）
  int baseColorSamples() const {
    switch (photometric) {
      case PHOTOMETRIC_RGB:
        return 3;
      case PHOTOMETRIC_SEPARATED:
        return 4;  // CMYK
      case PHOTOMETRIC_MINISBLACK:
      case PHOTOMETRIC_MINISWHITE:
        return 1;
      default:
        return samplesPerPixel - static_cast<int>(extraSamples.size());
    }
  }
};

struct TiffRawData {
  // 原始像素字节流
  // 布局：
  //   PLANARCONFIG_CONTIG :
  //     [S0 S1 S2 ...][S0 S1 S2 ...]
  //   PLANARCONFIG_SEPARATE :
  //     [plane0][plane1][plane2]...
  std::vector<uint8_t> buffer;

  // 每一行的字节数（TIFFScanlineSize）
  uint32_t bytesPerRow = 0;
};

struct TiffImage {
  TiffMeta meta;    // TIFF 标签信息
  TiffRawData raw;  // 原始像素数据

  // ---------------- 便捷方法 ----------------

  // 总通道数
  int channelCount() const { return meta.samplesPerPixel; }

  // 是否是 RGB 显示友好格式
  bool isRGBLike() const {
    return meta.photometric == PHOTOMETRIC_RGB && meta.bitsPerSample == 8;
  }

  // ExtraSamples 数量
  int extraSampleCount() const {
    return static_cast<int>(meta.extraSamples.size());
  }
};

static inline bool isAlphaSample(uint16_t type) {
  return type == EXTRASAMPLE_ASSOCALPHA || type == EXTRASAMPLE_UNASSALPHA;
}

int inline findAlphaExtraIndex(const TiffMeta& meta) {
  for (size_t i = 0; i < meta.extraSamples.size(); ++i) {
    if (isAlphaSample(meta.extraSamples[i])) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

inline const char* photometricToString(uint16_t p) {
  switch (p) {
    case PHOTOMETRIC_RGB:
      return "(RGB)";
    case PHOTOMETRIC_SEPARATED:
      return "(CMYK)";
    case PHOTOMETRIC_MINISBLACK:
      return "(Gray)";
    default:
      return "(Other)";
  }
}

inline const char* extraSampleToString(uint16_t v) {
  switch (v) {
    case EXTRASAMPLE_UNSPECIFIED:
      return "(UNSPECIFIED)";
    case EXTRASAMPLE_ASSOCALPHA:
      return "(ASSOC_ALPHA)";
    case EXTRASAMPLE_UNASSALPHA:
      return "(UNASSOC_ALPHA)";
    default:
      return "(UNKNOWN)";
  }
}

inline void dumpTiffMeta(const TiffMeta& m) {
  DEBUG << "[Meta]";
  DEBUG << "Size           :" << m.width << "x" << m.height;
  DEBUG << "SamplesPerPixel:" << m.samplesPerPixel;
  DEBUG << "BitsPerSample  :" << m.bitsPerSample;
  DEBUG << "Photometric    :" << m.photometric
        << photometricToString(m.photometric);
  DEBUG << "PlanarConfig   :" << m.planarConfig;
  DEBUG << "Compression    :" << m.compression;
  DEBUG << "Orientation    :" << m.orientation;
  DEBUG << "Resolution     :" << m.xResolution << "x" << m.yResolution
        << "unit=" << m.resolutionUnit;

  if (m.extraSamples.empty()) {
    DEBUG << "ExtraSamples   : none";
  } else {
    DEBUG << "ExtraSamples   : count =" << m.extraSamples.size();
    for (size_t i = 0; i < m.extraSamples.size(); ++i) {
      DEBUG << "  [" << i << "] = " << m.extraSamples[i]
            << extraSampleToString(m.extraSamples[i]);
    }
  }
}

inline void dumpTiffChannels(const TiffImage& img) {
  DEBUG << "[Channels]";

  const uint16_t spp = img.meta.samplesPerPixel;
  const uint16_t extra = static_cast<uint16_t>(img.meta.extraSamples.size());
  const uint16_t base = spp - extra;

  DEBUG << "TotalChannels  :" << spp;
  DEBUG << "BaseChannels   :" << base;
  DEBUG << "ExtraChannels  :" << extra;

  // 基础通道
  if (img.meta.photometric == PHOTOMETRIC_RGB) {
    DEBUG << "BaseType       : RGB";
    if (base >= 3) {
      DEBUG << "  0: R";
      DEBUG << "  1: G";
      DEBUG << "  2: B";
    }
  } else if (img.meta.photometric == PHOTOMETRIC_SEPARATED) {
    DEBUG << "BaseType       : CMYK";
    if (base >= 4) {
      DEBUG << "  0: C";
      DEBUG << "  1: M";
      DEBUG << "  2: Y";
      DEBUG << "  3: K";
    }
  }

  // Extra 通道
  for (size_t i = 0; i < img.meta.extraSamples.size(); ++i) {
    const uint16_t t = img.meta.extraSamples[i];
    DEBUG << "Extra[" << i << "]    :" << extraSampleToString(t);
  }
}

inline void dumpTiffRaw(const TiffImage& img) {
  DEBUG << "[RawData]";

  const size_t expect = static_cast<size_t>(img.meta.width) * img.meta.height *
                        img.meta.samplesPerPixel * (img.meta.bitsPerSample / 8);

  DEBUG << "BufferSize     :" << img.raw.buffer.size();
  DEBUG << "ExpectedSize   :" << expect;

  if (img.raw.buffer.size() != expect) {
    DEBUG << "!!! SIZE MISMATCH !!!";
  }
}

inline void dumpTiffImageInfo(const TiffImage& img) {
  DEBUG << "================ TIFF INFO BEGIN ================";

  dumpTiffMeta(img.meta);
  dumpTiffChannels(img);
  dumpTiffRaw(img);

  DEBUG << "================ TIFF INFO END ==================";
}
#endif  // TIFFIMAGE_H
