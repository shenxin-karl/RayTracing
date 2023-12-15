#pragma once
#include <cstdint>

class Mesh;
class Transform;
class StandardMaterial;

// clang-format off
union RenderObjectKey {
	uint64_t key;
	struct {
		uint16_t	renderGroup;
		uint16_t	materialId;
		float		depthSqr;
	};
};

struct RenderObject {
	RenderObjectKey		 key		= {};
	Mesh				*pMesh		= nullptr;
	StandardMaterial	*pMaterial	= nullptr;
	Transform			*pTransform	= nullptr;
};


// clang-format off
