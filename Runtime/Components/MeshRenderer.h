#pragma once
#include "Component.h"
#include "D3d12/D3dStd.h"
#include "RenderObject/RenderObject.h"
#include "RenderObject/VertexSemantic.hpp"

class Mesh;
class Material;
class Scene;

class MeshRenderer : public Component {
    DECLARE_CLASS(MeshRenderer);
public:
    MeshRenderer();
    void SetMesh(std::shared_ptr<Mesh> pMesh);
    void SetMaterial(std::shared_ptr<Material> pMaterial);
    auto GetMesh() const -> const std::shared_ptr<Mesh> & {
	    return _pMesh;
    }
    auto GetMaterial() const -> const std::shared_ptr<Material> & {
	    return _pMaterial;
    }
    auto GetASInstance() const -> const dx::ASInstance & {
	    return _instanceData;
    }
public:
    void OnRemoveFormScene() override;
    void OnAddToScene() override;
    void OnAddToGameObject() override;
    void OnPreRender() override;
    // Check if the ray tracing acceleration structure validity
    bool CheckASInstanceValidity(dx::AsyncASBuilder *pAsyncAsBuilder);
    bool PrepareAccelerationStructure();
private:
    void CommitRenderObject();
private:
    struct CachedRenderData {
        SemanticMask meshSemanticMask;
        bool shouldRender;
        RenderObject renderObject;
    };
    struct CachedASInstanceData {
        bool dirty = true;
    };
private:
    // clang-format off
	std::shared_ptr<Mesh>		_pMesh;
	std::shared_ptr<Material>	_pMaterial;
    Scene                      *_pCurrentScene;
	CachedRenderData			_renderData;
    dx::ASInstance              _instanceData;
    // clang-format on
};
