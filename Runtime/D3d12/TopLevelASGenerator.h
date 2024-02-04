#pragma once
#include "D3dStd.h"
#include "AccelerationStructure.h"
#include <glm/glm.hpp>

namespace dx {

class TopLevelASGenerator : private NonCopyable {
public:
    TopLevelASGenerator();
    void AddInstance(const ASInstance &instance);
    void AddInstance(ID3D12Resource *pBottomLevelAs,
        const glm::mat4x4 &transform,
        uint32_t instanceID,
        uint32_t hitGroupIndex,
        uint16_t instanceMask = 0xff);
    // If you're building from an old accelerated structure, you need to provide pPreviousResult
    auto CommitBuildCommand(IASBuilder *pASBuilder, TopLevelAS *pPreviousResult = nullptr) -> SharedPtr<TopLevelAS>;


    struct BuildDesc {
        Device *pDevice;
	    ComputeContext *pComputeContext;
        SharedPtr<Buffer> &pInstanceBuffer; // ����ʹ�õ� InstanceBuffer, Build �����ڻ��Զ�ά���������á�Ҫ���� Dynamic ���͵�
        SharedPtr<Buffer> &pScratchBuffer;  // ���������е���ʱ���� buffer, Build �����ڻ��Զ�ά���������á�Ҫ���� Static ���͵�
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    };
    auto Build(const BuildDesc &buildArgs) -> SharedPtr<TopLevelAS>;

    auto GetInstanceCount() const -> size_t {
        return _instances.size();
    }
    void SetInstances(std::vector<ASInstance> instances) {
        _instances = std::move(instances);
    }
private:
    std::vector<ASInstance> _instances;
};

}    // namespace dx
