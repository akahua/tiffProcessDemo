#ifndef PSTEMPLATE_H
#define PSTEMPLATE_H

#include "tiffimage.h"
#include <cstdint>
#include <string>
#include <tiffio.h>
#include <vector>
static bool replaceBytes(uint8_t *p, size_t n, const uint8_t *from,
                         size_t fromN, const uint8_t *to, size_t toN) {
  if (fromN != toN || fromN == 0 || n < fromN)
    return false;
  bool changed = false;
  for (size_t i = 0; i + fromN <= n; ++i) {
    if (std::memcmp(p + i, from, fromN) == 0) {
      std::memcpy(p + i, to, toN);
      changed = true;
    }
  }
  return changed;
}

static bool patch1045_W1W2_to_A1A2(uint8_t *data, uint32_t size) {
  bool changed = false;

  // UTF-16BE
  const uint8_t W1_BE[] = {0x00, 0x57, 0x00, 0x31};
  const uint8_t W2_BE[] = {0x00, 0x57, 0x00, 0x32};
  const uint8_t A1_BE[] = {0x00, 0x41, 0x00, 0x31};
  const uint8_t A2_BE[] = {0x00, 0x41, 0x00, 0x32};

  // UTF-16LE
  const uint8_t W1_LE[] = {0x57, 0x00, 0x31, 0x00};
  const uint8_t W2_LE[] = {0x57, 0x00, 0x32, 0x00};
  const uint8_t A1_LE[] = {0x41, 0x00, 0x31, 0x00};
  const uint8_t A2_LE[] = {0x41, 0x00, 0x32, 0x00};

  changed |=
      replaceBytes(data, size, W1_BE, sizeof(W1_BE), A1_BE, sizeof(A1_BE));
  changed |=
      replaceBytes(data, size, W2_BE, sizeof(W2_BE), A2_BE, sizeof(A2_BE));
  changed |=
      replaceBytes(data, size, W1_LE, sizeof(W1_LE), A1_LE, sizeof(A1_LE));
  changed |=
      replaceBytes(data, size, W2_LE, sizeof(W2_LE), A2_LE, sizeof(A2_LE));

  return changed;
}

// 修改 1006：把第2/3个名字改为 A1/A2（不改长度）
static bool patch1006_W1W2_to_A1A2(uint8_t *data, uint32_t size) {
  size_t pos = 0;
  int idx = 0;
  bool changed = false;

  while (pos < size) {
    if (pos + 1 > size)
      break;
    uint8_t len = data[pos++];

    if (pos + len > size)
      break;
    uint8_t *s = data + pos;

    // idx=0: 透明度（6字节GBK）
    // idx=1: W1 -> A1
    // idx=2: W2 -> A2
    if (len == 2 && idx == 1) {
      s[0] = 'A';
      s[1] = '1';
      changed = true;
    }
    if (len == 2 && idx == 2) {
      s[0] = 'A';
      s[1] = '2';
      changed = true;
    }

    pos += len;

    // ✅ 关键修正：1006 里是“内容长度 len”对齐到偶数
    if ((len & 1u) != 0) {
      if (pos + 1 > size)
        break;
      pos += 1;
    }

    idx++;
  }

  return changed;
}

// 在 34377 blob 中找到 rid=1006，并 patch
static bool patchPs34377_renameW1W2(std::vector<uint8_t> &ps34377) {
  if (ps34377.empty())
    return false;

  size_t pos = 0;
  const size_t len = ps34377.size();
  int hit = 0;

  bool patched1006 = false;
  bool patched1045 = false;
  while (pos + 4 <= len) {
    const size_t blockStart = pos;

    // signature
    if (pos + 4 > len)
      break;
    if (std::memcmp(ps34377.data() + pos, "8BIM", 4) != 0)
      break;
    pos += 4;

    // resource id
    if (pos + 2 > len)
      break;
    uint16_t rid = readBE16(ps34377.data() + pos);
    pos += 2;

    // Pascal name
    if (pos + 1 > len)
      break;
    uint8_t nameLen = ps34377[pos++];
    if (pos + nameLen > len)
      break;
    pos += nameLen;
    if (((1u + (unsigned)nameLen) & 1u) != 0) { // pad
      if (pos + 1 > len)
        break;
      pos += 1;
    }

    // data size
    if (pos + 4 > len)
      break;
    uint32_t dataSize = readBE32(ps34377.data() + pos);
    pos += 4;

    if (pos + dataSize > len)
      break;
    size_t dataOff = pos;
    pos += dataSize;
    if ((dataSize & 1u) != 0) { // pad
      if (pos + 1 > len)
        break;
      pos += 1;
    }
    if (rid == 1045) {
      patched1045 = patch1045_W1W2_to_A1A2(ps34377.data() + dataOff, dataSize);
      DEBUG << "[PsTemplate] patch 1045 W1/W2 -> A1/A2 :"
            << (patched1045 ? "OK" : "FAILED");
    }

    if (rid == 1006) {
      patched1006 = patch1006_W1W2_to_A1A2(ps34377.data() + dataOff, dataSize);
      DEBUG << "[PsTemplate] patch 1006 W1/W2 -> A1/A2 :"
            << (patched1006 ? "OK" : "FAILED");
    }

    if (pos <= blockStart)
      break;
  }
  if (patched1045 && patched1006)
    return true;
  (void)hit;
  return false;
}

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
    // bool changed = patchPs34377_renameW1W2(ps34377);

    dumpPsFlag(ps34377);
    TIFFClose(tif);
    return true;
  }
};

#endif // PSTEMPLATE_H
