#pragma once
#include "Foundation/Singleton.hpp"
#include "Foundation/NamespeceAlias.h"

namespace dx {
class Texture;
class UploadHeap;
class IImageLoader;
}    // namespace dx

class TextureManager : public Singleton<TextureManager> {
public:
    void OnCreate();
    void OnDestroy();
    auto LoadFromFile(stdfs::path path, dx::UploadHeap *pUploadHeap, bool makeSRGB = false) -> std::shared_ptr<dx::Texture>;
    static auto UploadTexture(dx::IImageLoader *pLoader, dx::UploadHeap *pUploadHeap, bool makeSRGB = false) -> std::shared_ptr<dx::Texture>;
private:
    using TextureMap = std::unordered_map<stdfs::path, std::shared_ptr<dx::Texture>>;
    TextureMap _textureMap;
};