#pragma once
#include <cstdint>

class Material;
class Mesh;
struct RayTracingGeometry {
	uint32_t  instanceID;
    uint32_t  hitGroupIndex : 24;
    uint32_t  instanceMask  : 8;
    Mesh     *pMesh;
    Material *pMaterial;
};