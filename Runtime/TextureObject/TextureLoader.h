#pragma once
#include "D3d12/DescriptorHandle.h"
#include "Foundation/Singleton.hpp"
#include "Foundation/NamespeceAlias.h"
#include "Foundation/Memory/SharedPtr.hpp"

namespace dx {
class Texture;
class UploadHeap;
class IImageLoader;
}    // namespace dx

class TextureLoader : public NonCopyable {
public:
    auto LoadFromFile(stdfs::path path, bool forceSRGB = false) -> SharedPtr<dx::Texture>;
    auto GetSRV2D(const dx::Texture *pTexture) -> dx::SRV;
    auto GetSRVCube(const dx::Texture *pTexture) -> dx::SRV;
    static auto UploadTexture(dx::IImageLoader *pLoader, bool forceSRGB = false) -> SharedPtr<dx::Texture>;
private:
    using TextureMap = std::unordered_map<stdfs::path, SharedPtr<dx::Texture>>;
    using SRV2DMap = std::unordered_map<const dx::Texture *, dx::SRV>;
    using SRVCubeMap = std::unordered_map<const dx::Texture *, dx::SRV>;
private:
    // clang-format off
    TextureMap _textureMap;
    SRV2DMap   _srv2DMap;
    SRVCubeMap _srvCubeMap;
    // clang-format on
};