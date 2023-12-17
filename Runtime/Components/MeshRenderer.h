#pragma once
#include "Component.h"
#include "RenderObject/RenderObject.h"
#include "RenderObject/VertexSemantic.hpp"

class Mesh;
class StandardMaterial;
class Scene;

class MeshRenderer : public Component {
	DECLARE_CLASS(MeshRenderer);
public:
	MeshRenderer();
	void SetMesh(std::shared_ptr<Mesh> pMesh);
	void SetMaterial(std::shared_ptr<StandardMaterial> pMaterial);
public:
	void OnRemoveFormScene() override;
	void OnAddToScene() override;
	void OnPreRender() override;
private:
	struct CachedRenderData {
		SemanticMask	  meshSemanticMask;
		Scene			 *pCurrentScene;
		bool			  shouldRender;
		RenderObject	  renderObject;
	};
private:
	// clang-format off
	std::shared_ptr<Mesh>				_pMesh;
	std::shared_ptr<StandardMaterial>	_pMaterial;
	CachedRenderData					_renderData;
	// clang-format on
};
