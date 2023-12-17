#pragma once
#include <cstdint>

class Mesh;
class Transform;
class StandardMaterial;

// clang-format off
struct RenderObject {
	Mesh				*pMesh		= nullptr;
	StandardMaterial	*pMaterial	= nullptr;
	Transform			*pTransform	= nullptr;
};


// clang-format off
