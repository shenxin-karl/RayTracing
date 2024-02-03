#pragma once
#include <cstdint>

class Material;
class Mesh;

struct InstanceMask {
    enum {
        // clang-format off
		eNone      = 0,
        eGeneric   = 1 << 0,
        eAlphaTest = 1 << 1,
        // clang-format on
    };
};

struct RayTracingGeometry {
    uint32_t instanceID;
    uint32_t hitGroupIndex : 24;
    uint32_t instanceMask  : 8;
    Mesh *pMesh;
    Material *pMaterial;
};