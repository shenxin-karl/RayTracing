#include "GLTFLoader.h"
#include "Foundation/Formatter.hpp"
#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Components/Transform.h"
#include "D3d12/IImageLoader.h"
#include "Object/GameObject.h"
#include "Renderer/GfxDevice.h"
#include "RenderObject/Mesh.h"
#include "RenderObject/StandardMaterial/StandardMaterial.h"
#include "RenderObject/VertexSemantic.hpp"
#include "TextureObject/DDSLoader.h"
#include "TextureObject/TextureManager.h"
#include "TextureObject/WICLoader.h"

bool GLTFLoader::Load(stdfs::path path, int flag) {
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    _pAiScene = importer.ReadFile(path.string(), flag);
    if (_pAiScene == nullptr || _pAiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || _pAiScene->mRootNode == nullptr) {
        _errorMessage = fmt::format("Load {} error: {}", path, importer.GetErrorString());
        return false;
    }

    stdfs::path directory = path.remove_filename();
    _materials.resize(_pAiScene->mNumMaterials);
    std::vector<bool> flags(_pAiScene->mNumMaterials, false);

    for (size_t i = 0; i < _pAiScene->mNumMeshes; ++i) {
        const aiMesh *pAiMesh = _pAiScene->mMeshes[i];
        const aiMaterial *pAiMaterial = _pAiScene->mMaterials[pAiMesh->mMaterialIndex];
        if (flags[pAiMesh->mMaterialIndex]) {
            continue;
        }
        _materials[pAiMesh->mMaterialIndex].Create(directory, _pAiScene, pAiMaterial);
        flags[pAiMesh->mMaterialIndex] = true;
    }

    _pRootGameObject = RecursiveBuildGameObject(_pAiScene->mRootNode);
    return true;
}

auto GLTFLoader::GetRootGameObject() const -> SharedPtr<GameObject> {
    return _pRootGameObject;
}

auto GLTFLoader::RecursiveBuildGameObject(aiNode *pAiNode) -> SharedPtr<GameObject> {
    SharedPtr<GameObject> pGameObject = GameObject::Create();
    pGameObject->SetName(pAiNode->mName.C_Str());
    if (pAiNode->mNumMeshes == 1) {
        unsigned int meshIdx = pAiNode->mMeshes[0];
        aiMesh *pAiMesh = _pAiScene->mMeshes[meshIdx];
        pGameObject->AddComponent(BuildMeshRenderer(meshIdx, pAiMesh));
    } else {
        for (size_t i = 0; i < pAiNode->mNumMeshes; ++i) {
            SharedPtr<GameObject> pChild = GameObject::Create();
            pChild->SetName(pGameObject->GetName() + fmt::format("_MeshRenderer_{}", i));
            pGameObject->AddChild(pChild);
            unsigned int meshIdx = pAiNode->mMeshes[0];
            aiMesh *pAiMesh = _pAiScene->mMeshes[meshIdx];
            pGameObject->AddComponent(BuildMeshRenderer(meshIdx, pAiMesh));
        }
    }

    aiVector3D scale;
    aiVector3D position;
    aiQuaternion rotate;
    pAiNode->mTransformation.Decompose(scale, rotate, position);
    Transform *pTransform = pGameObject->GetTransform();
    pTransform->SetLocalTRS(glm::vec3{position.x, position.y, position.z},
        glm::quat{rotate.w, rotate.x, rotate.y, rotate.z},
        glm::vec3(scale.x, scale.y, scale.z));

    for (size_t i = 0; i < pAiNode->mNumChildren; ++i) {
        auto pChild = RecursiveBuildGameObject(pAiNode->mChildren[i]);
        pGameObject->AddChild(pChild);
    }
    return pGameObject;
}

auto GLTFLoader::BuildMeshRenderer(size_t meshIndex, aiMesh *pAiMesh) -> SharedPtr<MeshRenderer> {
    std::shared_ptr<Mesh> pMesh = BuildMesh(pAiMesh);
    std::shared_ptr<StandardMaterial> pMaterial = BuildMaterial(pAiMesh->mMaterialIndex);
    SharedPtr<MeshRenderer> pMeshRenderer = MakeShared<MeshRenderer>();
    pMeshRenderer->SetMaterial(pMaterial);
    pMeshRenderer->SetMesh(pMesh);
    return pMeshRenderer;
}

auto GLTFLoader::BuildMesh(aiMesh *pAiMesh) -> std::shared_ptr<Mesh> {
    std::shared_ptr<Mesh> pMesh = std::make_shared<Mesh>();
    SemanticMask mask = SemanticMask::eVertex;
    mask = SetOrClearFlags(mask, SemanticMask::eNormal, pAiMesh->HasNormals());
    mask = SetOrClearFlags(mask, SemanticMask::eTangent, pAiMesh->HasTangentsAndBitangents());
    mask = SetOrClearFlags(mask, SemanticMask::eTexCoord0, pAiMesh->HasTextureCoords(0));
    mask = SetOrClearFlags(mask, SemanticMask::eColor, pAiMesh->HasVertexColors(0));

    size_t numVertices = pAiMesh->mNumVertices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> tangents;
    std::vector<glm::vec2> uv0;
    std::vector<glm::vec4> colors;
    std::vector<uint16_t> indices;

    vertices.reserve(numVertices);
    normals.reserve(HasFlag(mask, SemanticMask::eNormal) ? numVertices : 0);
    tangents.reserve(HasFlag(mask, SemanticMask::eTangent) ? numVertices : 0);
    uv0.reserve(HasFlag(mask, SemanticMask::eTexCoord0) ? numVertices : 0);
    colors.reserve(HasFlag(mask, SemanticMask::eColor) ? numVertices : 0);

    for (size_t i = 0; i < numVertices; ++i) {
        const aiVector3D &pos = pAiMesh->mVertices[i];
        vertices.emplace_back(pos.x, pos.y, pos.z);
        if (HasFlag(mask, SemanticMask::eNormal) && pAiMesh->mNormals) {
            const aiVector3D &nrm = pAiMesh->mNormals[i];
            normals.emplace_back(nrm.x, nrm.y, nrm.z);
        }
        if (HasFlag(mask, SemanticMask::eTangent) && pAiMesh->mTangents) {
            const aiVector3D &tan = pAiMesh->mTangents[i];
            float det = 1.f;
            if (pAiMesh->mNormals && pAiMesh->mBitangents) {
                const aiVector3D &bit = pAiMesh->mBitangents[i];
                glm::vec3 N = normalize(normals[i]);
                glm::vec3 T(tan.x, tan.y, tan.z);
                glm::vec3 B(bit.x, bit.y, bit.z);
                det = dot(cross(N, T), B) < 0.f ? -1.f : 1.f;
            }
            tangents.emplace_back(tan.x, tan.y, tan.z, det);
        }
        if (HasFlag(mask, SemanticMask::eTexCoord0) && pAiMesh->mTextureCoords[0]) {
            const aiVector3D &tex0 = pAiMesh->mTextureCoords[0][i];
            uv0.emplace_back(tex0.x, tex0.y);
        }
        if (HasFlag(mask, SemanticMask::eColor) && pAiMesh->mColors[0]) {
            const aiColor4D &color = pAiMesh->mColors[0][i];
            colors.emplace_back(color.r, color.g, color.b, color.a);
        }
    }

    for (size_t i = 0; i < pAiMesh->mNumFaces; ++i) {
        for (size_t j = 0; j < pAiMesh->mFaces[i].mNumIndices; ++j) {
            indices.push_back(pAiMesh->mFaces[i].mIndices[j]);
        }
    }

    pMesh->Resize(mask, numVertices, indices.size());
    pMesh->SetVertices(vertices);
    if (HasFlag(mask, SemanticMask::eNormal)) {
        pMesh->SetNormals(normals);
    }
    if (HasFlag(mask, SemanticMask::eTangent)) {
        pMesh->SetTangents(tangents);
    }
    if (HasFlag(mask, SemanticMask::eColor)) {
        pMesh->SetColors(colors);
    }
    if (HasFlag(mask, SemanticMask::eTexCoord0)) {
        pMesh->SetUV0(uv0);
    }

    pMesh->UploadMeshData(false);
    return pMesh;
}

auto GLTFLoader::BuildMaterial(size_t materialIndex) -> std::shared_ptr<StandardMaterial> {
    Material &gltfMaterial = _materials[materialIndex];
    if (gltfMaterial.pStdMaterial != nullptr) {
        return gltfMaterial.pStdMaterial;
    }

    gltfMaterial.pStdMaterial = std::make_shared<StandardMaterial>();
    std::shared_ptr<StandardMaterial> &pMaterial = gltfMaterial.pStdMaterial;
    pMaterial->SetRenderGroup(gltfMaterial.renderGroup);
    pMaterial->SetCutoff(gltfMaterial.alphaCutoff);
    if (gltfMaterial.baseColorMap.IsValid()) {
        std::shared_ptr<dx::Texture> pBaseTexture = gltfMaterial.LoadTexture(gltfMaterial.baseColorMap, true);
        pMaterial->SetTextures(StandardMaterial::eAlbedoTex, pBaseTexture);
    }
    if (gltfMaterial.normalMap.IsValid()) {
        std::shared_ptr<dx::Texture> pNormalMap = gltfMaterial.LoadTexture(gltfMaterial.normalMap, false);
        pMaterial->SetTextures(StandardMaterial::eNormalTex, pNormalMap);
    }
    if (gltfMaterial.emissionMap.IsValid()) {
        std::shared_ptr<dx::Texture> pEmissionMap = gltfMaterial.LoadTexture(gltfMaterial.emissionMap, false);
        pMaterial->SetTextures(StandardMaterial::eEmissionTex, pEmissionMap);
    }
    if (gltfMaterial.metalnessRoughnessMap.IsValid()) {
        std::shared_ptr<dx::Texture> pMetalRoughnessMap = gltfMaterial.LoadTexture(gltfMaterial.metalnessRoughnessMap, true);
        pMaterial->SetTextures(StandardMaterial::eMetalRoughnessTex, pMetalRoughnessMap);
    }
    if (gltfMaterial.ambientOcclusionMap.IsValid()) {
        std::shared_ptr<dx::Texture> pAmbientOcclusionMap = gltfMaterial.LoadTexture(gltfMaterial.ambientOcclusionMap, false);
        pMaterial->SetTextures(StandardMaterial::eAmbientOcclusionTex, pAmbientOcclusionMap);
    }
    return pMaterial;
}

void GLTFLoader::Material::Create(stdfs::path directory, const aiScene *pAiScene, const aiMaterial *pAiMaterial) {
    if (!ProcessTexture(baseColorMap, directory, pAiScene, pAiMaterial, aiTextureType_BASE_COLOR)) {
        ProcessTexture(baseColorMap, directory, pAiScene, pAiMaterial, aiTextureType_DIFFUSE);
    }
    if (!ProcessTexture(normalMap, directory, pAiScene, pAiMaterial, aiTextureType_NORMAL_CAMERA)) {
        ProcessTexture(normalMap, directory, pAiScene, pAiMaterial, aiTextureType_NORMALS);
    }
    if (!ProcessTexture(emissionMap, directory, pAiScene, pAiMaterial, aiTextureType_EMISSION_COLOR)) {
        ProcessTexture(emissionMap, directory, pAiScene, pAiMaterial, aiTextureType_EMISSIVE);
    }
    ProcessTexture(metalnessRoughnessMap, directory, pAiScene, pAiMaterial, aiTextureType_UNKNOWN);
    if (!ProcessTexture(ambientOcclusionMap, directory, pAiScene, pAiMaterial, aiTextureType_AMBIENT_OCCLUSION)) {
        ProcessTexture(ambientOcclusionMap, directory, pAiScene, pAiMaterial, aiTextureType_LIGHTMAP);
    }

    aiString aiAlphaMode;
    if (pAiMaterial->Get(AI_MATKEY_GLTF_ALPHAMODE, aiAlphaMode) == aiReturn_SUCCESS) {
        if (std::strcmp(aiAlphaMode.data, "MASK") == 0) {
            float maskThreshold = 0.0;
            renderGroup = RenderGroup::eAlphaTest;
            if (pAiMaterial->Get(AI_MATKEY_GLTF_ALPHACUTOFF, maskThreshold) == aiReturn_SUCCESS) {
                alphaCutoff = maskThreshold;
            }
        } else if (std::strcmp(aiAlphaMode.data, "OPAQUE") == 0) {
            renderGroup = RenderGroup::eOpaque;
        } else if (std::strcmp(aiAlphaMode.data, "BLEND") == 0) {
            renderGroup = RenderGroup::eTransparent;
        }
    }
}

auto GLTFLoader::Material::LoadTexture(Texture &texture, bool makeSRGB) -> std::shared_ptr<dx::Texture> {
    if (auto it = textureMap.find(&texture); it != textureMap.end()) {
        return it->second;
    }

    bool loadSuccess = false;
    std::unique_ptr<dx::IImageLoader> pImageLoader;
    if (texture.pTextureData != nullptr) {
	    if (texture.extension == "DDS" || texture.extension == "dds") {
		    std::unique_ptr<MemoryDDSLoader> pLoader = std::make_unique<MemoryDDSLoader>();
            loadSuccess = pLoader->Load(texture.pTextureData.get(), texture.textureDataSize, 0.f);
            pImageLoader = std::move(pLoader);
	    } else {
		    std::unique_ptr<WICLoader> pLoader = std::make_unique<WICLoader>();
            loadSuccess = pLoader->Load(texture.pTextureData.get(), texture.textureDataSize, 0.f);
            pImageLoader = std::move(pLoader);
	    }
    } else {
	    if (texture.extension == "DDS" || texture.extension == "dds") {
		    std::unique_ptr<FileDDSLoader> pLoader = std::make_unique<FileDDSLoader>();
            loadSuccess = pLoader->Load(texture.path, 0.f);
            pImageLoader = std::move(pLoader);
	    } else {
		    std::unique_ptr<WICLoader> pLoader = std::make_unique<WICLoader>();
            loadSuccess = pLoader->Load(texture.path, 0.f);
            pImageLoader = std::move(pLoader);
	    }
    }

    if (!loadSuccess) {
	    return nullptr;
    }

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    std::shared_ptr<dx::Texture> pTexture = TextureManager::GetInstance()->UploadTexture(
	    pImageLoader.get(), pGfxDevice->GetUploadHeap(), makeSRGB);

    textureMap[&texture] = pTexture;
    return pTexture;
}

bool GLTFLoader::Material::ProcessTexture(Texture &texture,
    const stdfs::path &directory,
    const aiScene *pAiScene,
    const aiMaterial *pAiMaterial,
    aiTextureType type) const {

    aiString path;
    if (pAiMaterial->GetTexture(type, 0, &path) != aiReturn_SUCCESS) {
        return false;
    }

    const aiTexture *pAiTexture = pAiScene->GetEmbeddedTexture(path.C_Str());
    if (pAiTexture != nullptr) {
        texture.path = directory / path.C_Str();
        texture.textureDataSize = pAiTexture->mWidth;
        texture.extension = pAiTexture->achFormatHint;
        if (pAiTexture->pcData != nullptr) {
            texture.pTextureData = std::make_shared<uint8_t[]>(pAiTexture->mWidth);
            Assert(pAiTexture->mWidth == 0);
            std::memcpy(texture.pTextureData.get(), pAiTexture->pcData, pAiTexture->mWidth);
        }
    } else {
        if (path.length > 0) {
            texture.path = directory / path.C_Str();
            std::error_code errorCode;
            texture.fileExist = stdfs::exists(texture.path, errorCode);
        }
    }
    return texture.IsValid();
}
