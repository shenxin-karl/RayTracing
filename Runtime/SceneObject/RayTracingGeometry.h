#pragma once
#include <cstdint>
#include "D3d12/D3dStd.h"
#include "Foundation/Memory/SharedPtr.hpp"
#include "D3d12/AccelerationStructure.h"

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

class RayTracingGeometry {
    friend class SceneRayTracingASManager;
    RayTracingGeometry() = default;
public:
    RayTracingGeometry(const RayTracingGeometry &) = default; 
    void SetInstanceID(uint32_t instanceID) {
        _instanceID = instanceID;
    }
    void SetHitGroupIndex(uint32_t hitGroupIndex) {
        _hitGroupIndex = hitGroupIndex;
    }
    void SetInstanceMask(uint8_t instanceMask) {
        _instanceMask = instanceMask;
    }
    void SetInstanceFlags(dx::RayTracingInstanceFlags flags) {
        _instanceFlag = flags;
    }
    void SetTransform(const glm::mat4x4 &transform) {
	    _transform = transform;
    }
    auto GetMesh() const -> const Mesh * {
	    return _pMesh;
    }
    auto GetMaterial() const -> const Material * {
	    return _pMaterial;
    }
    auto GetInstanceID() const -> uint32_t {
	    return _instanceID;
    }
    auto GetHitGroupIndex() const -> uint32_t {
	    return _hitGroupIndex;
    }
    auto GetInstanceMask() const -> uint8_t {
	    return _instanceMask;
    }
    auto GetInstanceFlags() const -> dx::RayTracingInstanceFlags {
	    return _instanceFlag;
    }
    auto GetTransform() const -> const glm::mat4x4 & {
	    return _transform;
    }
private:
    // clang-format off
    Mesh                       *_pMesh;
    Material                   *_pMaterial;
    uint32_t                    _instanceID;
    uint32_t                    _hitGroupIndex : 24;
    uint32_t                    _instanceMask  : 8;
    dx::RayTracingInstanceFlags _instanceFlag;
    glm::mat4x4                 _transform;
    // clang-format on
};

class RegionTopLevelAS : NonCopyable {
private:
    friend class SceneRayTracingASManager;
    RegionTopLevelAS() = default;
    static std::shared_ptr<RegionTopLevelAS> Create();
public:
    ~RegionTopLevelAS() = default;
    auto GetTopLevelAS() -> const SharedPtr<dx::TopLevelAS> & {
	    return _pTopLevelAS;
    }
    auto GetGeometries() const -> const std::vector<RayTracingGeometry> & {
	    return _geometries;
    }
    auto GetGeometries() -> std::vector<RayTracingGeometry> & {
	    return _geometries;
    }
private:
    // clang-format off
    std::vector<RayTracingGeometry> _geometries;
    SharedPtr<dx::TopLevelAS>       _pTopLevelAS;
    // clang-format on
};

inline std::shared_ptr<RegionTopLevelAS> RegionTopLevelAS::Create() {
	struct MakeRegionTopLevelAS : public RegionTopLevelAS {};
    return std::make_shared<MakeRegionTopLevelAS>();
}
