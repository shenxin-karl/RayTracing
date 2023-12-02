#include "WICLoader.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

WICLoader::~WICLoader() {
    if (_pData != nullptr) {
        free(_pData);
        _pData = nullptr;
    }
}

bool WICLoader::Load(const stdfs::path &filePath, float cutOff) {
    int32_t width, height, channels;
    _pData = reinterpret_cast<char *>(stbi_load(filePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha));

    // compute number of mips
    //
    uint32_t mipWidth = width;
    uint32_t mipHeight = height;
    uint32_t mipCount = 0;
    while (true) {
        mipCount++;
        if (mipWidth > 1) {
            mipWidth >>= 1;
        }
        if (mipHeight > 1) {
            mipHeight >>= 1;
        }
        if (mipWidth == 1 && mipHeight == 1) {
            break;
        }
    }

    // fill img struct
    _imageHeader.arraySize = 1;
    _imageHeader.width = width;
    _imageHeader.height = height;
    _imageHeader.depth = 1;
    _imageHeader.mipMapCount = mipCount;
    _imageHeader.bitCount = 32;
    _imageHeader.format = DXGI_FORMAT_R8G8B8A8_UNORM;

    _cutOff = cutOff;
    if (_cutOff < 1.0f) {
        _alphaTestCoverage = GetAlphaCoverage(width, height, 1.0f, static_cast<int>(255 * _cutOff));
    } else {
        _alphaTestCoverage = 1.0f;
    }

    return true;
}

auto WICLoader::GetImageHeader() const -> dx::ImageHeader {
    return _imageHeader;
}

void WICLoader::GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) {
    for (uint32_t y = 0; y < height; y++) {
        memcpy(static_cast<char *>(pDest) + y * stride, _pData + y * width, width);
    }
    MipImage(width / 4, height);
}

void WICLoader::MipImage(uint32_t width, uint32_t height) {
    //compute mip so next call gets the lower mip
    int offsetsX[] = {0, 1, 0, 1};
    int offsetsY[] = {0, 0, 1, 1};

    uint32_t *pImg = reinterpret_cast<uint32_t *>(_pData);

#define GetByte(color, component) (((color) >> (8 * (component))) & 0xff)
#define GetColor(ptr, x, y) (ptr[(x) + (y)*width])
#define SetColor(ptr, x, y, col) ptr[(x) + (y)*width / 2] = col;

    for (uint32_t y = 0; y < height; y += 2) {
        for (uint32_t x = 0; x < width; x += 2) {
            uint32_t ccc = 0;
            for (uint32_t c = 0; c < 4; c++) {
                uint32_t cc = 0;
                for (uint32_t i = 0; i < 4; i++) {
                    cc += GetByte(GetColor(pImg, x + offsetsX[i], y + offsetsY[i]), 3 - c);
                }
                ccc = (ccc << 8) | (cc / 4);
            }
            SetColor(pImg, x / 2, y / 2, ccc);
        }
    }

    // For cutouts we need to scale the alpha channel to match the coverage of the top MIP map
    // otherwise cutouts seem to get thinner when smaller mips are used
    // Credits: http://www.ludicon.com/castano/blog/articles/computing-alpha-mipmaps/
    if (_alphaTestCoverage < 1.0) {
        float ini = 0;
        float fin = 10;
        float mid = 0.f;
        for (int iter = 0; iter < 50; iter++) {
            mid = (ini + fin) / 2;
            float alphaPercentage = GetAlphaCoverage(width / 2, height / 2, mid, static_cast<int>(_cutOff * 255));
            if (fabs(alphaPercentage - _alphaTestCoverage) < .001) {
                break;
            }
            if (alphaPercentage > _alphaTestCoverage) {
                fin = mid;
            }
            if (alphaPercentage < _alphaTestCoverage) {
                ini = mid;
            }
        }
        ScaleAlpha(width / 2, height / 2, mid);
    }
}

void WICLoader::ScaleAlpha(uint32_t width, uint32_t height, float scale) {
    uint32_t *pImg = reinterpret_cast<uint32_t *>(_pData);
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint8_t *pPixel = reinterpret_cast<uint8_t *>(pImg++);
            int alpha = static_cast<int>(scale * static_cast<float>(pPixel[3]));
            if (alpha > 255) {
                alpha = 255;
            }
            pPixel[3] = alpha;
        }
    }
}

auto WICLoader::GetAlphaCoverage(uint32_t width, uint32_t height, float scale, int cutoff) const -> float {
    double val = 0;
    uint32_t *pImg = reinterpret_cast<uint32_t *>(_pData);
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint8_t *pPixel = reinterpret_cast<uint8_t *>(pImg++);
            int alpha = static_cast<int>(scale * static_cast<float>(pPixel[3]));
            if (alpha > 255) {
                alpha = 255;
            }
            if (alpha <= cutoff) {
                continue;
            }
            val += alpha;
        }
    }
    return static_cast<float>(val / (height * width * 255));
}
