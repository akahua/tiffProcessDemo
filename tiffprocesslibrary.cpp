#include "tiffprocesslibrary.h"

#include <tiffio.h>
#include <windows.h>

#include <string>
#include <vector>

static std::string WideToUtf8(const wchar_t* wstr) {
  if (!wstr) return {};
  int len =
      WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
  if (len <= 0) return {};
  std::string utf8(len - 1, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8.data(), len, nullptr, nullptr);
  return utf8;
}

static std::string WideToUtf8(std::wstring_view wstr) {
  if (wstr.empty()) return {};
  std::wstring tmp(wstr);
  return WideToUtf8(tmp.c_str());
}

extern "C" {

// 传统接口
int MchBmpTiffOut(LPBYTE pSrc, int nWidth, int nHeight, int nPixBits,
                  int nBytePerLine, int nCHcnt, wchar_t* szTiffFile) {
  if (!pSrc || nWidth <= 0 || nHeight <= 0 || nCHcnt < 4) return -1;
  if (nPixBits != 8) return -2;

  const int bytesPerPixel = (nPixBits * nCHcnt) / 8;
  if (nBytePerLine < nWidth * bytesPerPixel) return -5;

  std::string utf8Path = WideToUtf8(szTiffFile);
  TIFF* tif = TIFFOpen(utf8Path.c_str(), "w");
  if (!tif) return -3;

  const int samplesPerPixel = nCHcnt;
  const int spotCount = nCHcnt - 4;

  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, nWidth);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, nHeight);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, nPixBits);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, samplesPerPixel);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_SEPARATED);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
  TIFFSetField(tif, TIFFTAG_INKSET, INKSET_CMYK);

  if (spotCount > 0) {
    std::vector<uint16_t> extraSamples(spotCount, EXTRASAMPLE_UNSPECIFIED);
    TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, spotCount, extraSamples.data());
  }

  if (spotCount > 0) {
    std::string inkNames("Cyan\0Magenta\0Yellow\0Black\0", 26);
    for (int i = 0; i < spotCount; ++i) {
      inkNames += "Spot";
      inkNames += std::to_string(i + 1);
      inkNames.push_back('\0');
    }
    TIFFSetField(tif, TIFFTAG_INKNAMES, inkNames.size(), inkNames.c_str());
  }

  for (int y = 0; y < nHeight; ++y) {
    LPBYTE srcLine = pSrc + y * nBytePerLine;
    if (TIFFWriteScanline(tif, srcLine, y, 0) < 0) {
      TIFFClose(tif);
      return -4;
    }
  }

  TIFFClose(tif);
  return 0;
}

// 现代接口
int MchBmpTiffOut1(const std::uint8_t* data, int width, int height,
                   int bitsPerChannel, int bytesPerLine, int channelCount,
                   std::wstring_view tiffPath) {
  if (!data || width <= 0 || height <= 0 || channelCount < 4) return -1;
  if (bitsPerChannel != 8) return -2;

  const int bytesPerPixel = (bitsPerChannel * channelCount) / 8;
  if (bytesPerLine < width * bytesPerPixel) return -5;

  std::string utf8Path = WideToUtf8(tiffPath);

  TIFF* tif = TIFFOpen(utf8Path.c_str(), "w");
  if (!tif) return -3;

  const int samplesPerPixel = channelCount;
  const int spotCount = channelCount - 4;

  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bitsPerChannel);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, samplesPerPixel);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_SEPARATED);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
  TIFFSetField(tif, TIFFTAG_INKSET, INKSET_CMYK);

  if (spotCount > 0) {
    std::vector<uint16_t> extraSamples(spotCount, EXTRASAMPLE_UNSPECIFIED);
    TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, spotCount, extraSamples.data());
  }

  if (spotCount > 0) {
    std::string inkNames("Cyan\0Magenta\0Yellow\0Black\0", 26);
    for (int i = 0; i < spotCount; ++i) {
      inkNames += "Spot";
      inkNames += std::to_string(i + 1);
      inkNames.push_back('\0');
    }
    TIFFSetField(tif, TIFFTAG_INKNAMES, inkNames.size(), inkNames.c_str());
  }

  for (int y = 0; y < height; ++y) {
    const std::uint8_t* srcLine = data + y * bytesPerLine;
    if (TIFFWriteScanline(tif, const_cast<std::uint8_t*>(srcLine), y, 0) < 0) {
      TIFFClose(tif);
      return -4;
    }
  }

  TIFFClose(tif);
  return 0;
}
}
