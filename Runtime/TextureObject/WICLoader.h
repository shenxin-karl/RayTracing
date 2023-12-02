#pragma once
#include "D3d12/IImageLoader.h"

class WICLoader : public dx::IFileImageLoader {
public:
    WICLoader() = default;
    ~WICLoader() override;
public:
    bool Load(const stdfs::path &filePath, float cutOff) override;
    auto GetImageHeader() const -> dx::ImageHeader override;
    void GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) override;
private:
    void MipImage(uint32_t width, uint32_t height);
    void ScaleAlpha(uint32_t width, uint32_t height, float scale);
    auto GetAlphaCoverage(uint32_t width, uint32_t height, float scale, int cutoff) const -> float;
private:
    // clang-format off
	char               *_pData              = nullptr;
    float               _cutOff             = 0.f;
    float               _alphaTestCoverage  = 0.f;
    dx::ImageHeader     _imageHeader        = {};
    // clang-format on
};