#pragma once
#include <cstdint>
#include <dxgiformat.h>
#include "Foundation/NamespeceAlias.h"

namespace dx {

// Describes the image information
struct ImageHeader {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t arraySize;
    uint32_t mipMapCount;
    uint32_t bitCount;
    DXGI_FORMAT format;
};

class IImageLoader {
public:
    virtual ~IImageLoader() = default;
    virtual auto GetImageHeader() const -> ImageHeader = 0;
    virtual void GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) = 0;
};

class IFileImageLoader : public virtual IImageLoader {
public:
    virtual bool Load(const stdfs::path &filePath, float cutOff) = 0;
};

class IMemoryImageLoader : public virtual IImageLoader {
public:
    virtual bool Load(const uint8_t *pData, size_t dataSize, float cutOff) = 0;
};

}    // namespace dx
