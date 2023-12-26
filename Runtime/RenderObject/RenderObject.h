#pragma once
#include <cstdint>

class Mesh;
class Transform;
class Material;

// clang-format off
struct RenderObject {
	Mesh				*pMesh		= nullptr;
	Material	*pMaterial	= nullptr;
	Transform			*pTransform	= nullptr;
	uint16_t			 priority	= 0;
};


// clang-format off
