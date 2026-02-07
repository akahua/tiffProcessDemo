#ifndef PSTEMPLATE_H
#define PSTEMPLATE_H

#include <cstdint>
#include <string>
#include <tiffio.h>
#include <vector>
class PsTemplate {
public:
  std::vector<uint8_t> ps34377;
  uint16_t spp = 0;
  uint16_t extra = 0;

  bool load(const std::string &path) {
    TIFF *tif = TIFFOpen(path.c_str(), "r");
    if (!tif)
      return false;

    // 校验结构
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);

    uint16_t cnt = 0;
    uint16_t *ex = nullptr;
    if (TIFFGetField(tif, TIFFTAG_EXTRASAMPLES, &cnt, &ex))
      extra = cnt;

    // 读34377
    uint32_t n = 0;
    void *data = nullptr;
    if (!TIFFGetField(tif, TIFFTAG_PHOTOSHOP, &n, &data) || !data) {
      TIFFClose(tif);
      return false;
    }

    ps34377.resize(n);
    memcpy(ps34377.data(), data, n);

    TIFFClose(tif);
    return true;
  }
};

#endif // PSTEMPLATE_H
