#ifndef TIFFPROCESSLIBRARY_H
#define TIFFPROCESSLIBRARY_H

#include <string_view>
#ifdef _WIN32
#ifdef TIFFPROCESSLIBRARY_EXPORTS
#define TIFF_API __declspec(dllexport)
#else
#define TIFF_API __declspec(dllimport)
#endif
#else
#define TIFF_API
#endif

#include <cstdint>

typedef std::uint8_t BYTE;
typedef BYTE* LPBYTE;

extern "C" {
TIFF_API int MchBmpTiffOut(LPBYTE pSrc, int nWidth, int nHeight, int nPixBits,
                           int nBytePerLine, int nCHcnt, wchar_t* szTiffFile);

TIFF_API int MchBmpTiffOut1(const std::uint8_t* data, int width, int height,
                            int bitsPerChannel, int bytesPerLine,
                            int channelCount, std::wstring_view tiffPath);
}

#endif  // TIFFPROCESSLIBRARY_H
