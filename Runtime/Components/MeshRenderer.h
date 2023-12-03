#pragma once
#include "Component.h"

class Mesh;
class StandardMaterial;

class MeshRenderer : public Component {
	DECLARE_CLASS(MeshRenderer);
public:
	void OnRemoveFormGameObject() override;
	void OnRemoveFormScene() override;
private:
	// clang-format off
	std::shared_ptr<Mesh>				_pMesh;
	std::shared_ptr<StandardMaterial>	_pMaterial;
	// clang-format on
};
