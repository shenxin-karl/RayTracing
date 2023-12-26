#pragma once
#include <memory>
#include "Foundation/NonCopyable.h"
#include "Foundation/NamespeceAlias.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "Components/MeshRenderer.h"
#include "Foundation/Memory/SharedPtr.hpp"
#include "Foundation/ColorUtil.hpp"
#include "RenderObject/RenderGroup.hpp"
#include "TextureObject/TextureLoader.h"

namespace dx {
class Texture;
}

class GameObject;
class StandardMaterial;
class GLTFLoader : NonCopyable {
public:
    constexpr static int kDefaultLoadFlag = (aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded |
                                             aiProcess_OptimizeGraph);
    bool Load(stdfs::path path, int flag = kDefaultLoadFlag);
    auto GetRootGameObject() const -> SharedPtr<GameObject>;
private:
    struct Material;
    auto RecursiveBuildGameObject(aiNode *pAiNode) -> SharedPtr<GameObject>;
    auto BuildMeshRenderer(size_t meshIndex, aiMesh *pAiMesh) -> SharedPtr<MeshRenderer>;
    static auto BuildMesh(aiMesh *pAiMesh) -> std::shared_ptr<Mesh>;
    auto BuildMaterial(size_t materialIndex) -> std::shared_ptr<StandardMaterial>;
private:
    // clang-format off
    const aiScene              *_pAiScene = nullptr;
    std::string                 _errorMessage;
    std::vector<Material>       _materials;
    SharedPtr<GameObject>       _pRootGameObject;
    TextureLoader               _textureLoader;
    // clang-format on
};

struct GLTFLoader::Material {
    struct Texture {
        bool IsValid() const {
            return (!path.empty() && fileExist) || (pTextureData != nullptr && textureDataSize > 0);
        }
    public:
        // clang-format off
        bool                        fileExist       = false;
	    stdfs::path                 path            = {};
        std::string                 extension       = {};
        size_t                      textureDataSize = {};
        std::shared_ptr<uint8_t[]>  pTextureData    = {};
        // clang-format on
    };
public:
    void Create(TextureLoader *pTextureLoader, stdfs::path directory, const aiScene *pAiScene, const aiMaterial *pAiMaterial);
    auto LoadTexture(Texture &texture, bool makeSRGB) -> std::shared_ptr<dx::Texture>;
private:
    bool ProcessTexture(Texture &texture,
        const stdfs::path &directory,
        const aiScene *pAiScene,
        const aiMaterial *pAiMaterial,
        aiTextureType type) const;
public:
    uint16_t renderGroup = RenderGroup::eOpaque;    // 0 Opaque, 1 Alpha 2 Blend
    float alphaCutoff = 0.f;
    glm::vec4 albedo = Colors::White;
    Texture baseColorMap;
    Texture normalMap;
    Texture emissionMap;
    Texture metalnessRoughnessMap;
    Texture ambientOcclusionMap;
    TextureLoader *pTextureLoader = nullptr;
    std::shared_ptr<StandardMaterial> pStdMaterial;
    std::unordered_map<Texture *, std::shared_ptr<dx::Texture>> textureMap;
};