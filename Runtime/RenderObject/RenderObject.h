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
	uint16_t			 priority	= 0;
};


// clang-format off
