#pragma once
#include "D3d12/IImageLoader.h"

class WICLoader : public dx::IFileImageLoader, public dx::IMemoryImageLoader {
public:
    WICLoader() = default;
    ~WICLoader() override;
public:
    bool Load(const stdfs::path &filePath, float cutOff) override;
    bool Load(const uint8_t *pData, size_t dataSize, float cutOff) override;
    auto GetImageHeader() const -> dx::ImageHeader override;
    void GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) override;
private:
    void LoadInternal(int width, int height, float cutOff);
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