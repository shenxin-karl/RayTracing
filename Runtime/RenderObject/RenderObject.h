#pragma once
#include <cstdint>
#include "ConstantBufferHelper.h"

class Mesh;
class Transform;
class Material;

// clang-format off
struct RenderObject {
	const Mesh				*pMesh			= nullptr;
	const Material			*pMaterial		= nullptr;
	const Transform			*pTransform		= nullptr;
	cbuffer::CbPreObject	 cbPreObject	= {};
	uint16_t				 priority		= 0;
};

// clang-format off
