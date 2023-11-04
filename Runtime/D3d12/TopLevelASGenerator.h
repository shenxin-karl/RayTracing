#pragma once
#include "D3dUtils.h"
#include "AccelerationStructure.h"
#include <glm/glm.hpp>

namespace dx {

class TopLevelASGenerator : public NonCopyable {
    struct Instance {
        // clang-format off
		BottomLevelAS *pBottomLevelAs	 = nullptr;
		glm::mat3x4	   transform		 = glm::mat3x4(1.0);
		uint32_t	   instanceID		 = 0;	
		uint32_t	   hitGroupIndex	 = 0;
        // clang-format on
    };
public:
    TopLevelASGenerator();
    void AddInstance(const Instance &instance);
    void AddInstance(BottomLevelAS *pBottomLevelAs,
        const glm::mat3x4 &transform,
        uint32_t instanceID,
        uint32_t hitGroupIndex);
    void ComputeAsBufferSizes(ASBuilder *pUploadHeap, bool allowUpdate = false);
    // If you're building from an old accelerated structure, you need to provide pPreviousResult
    auto Generate(ASBuilder *pUploadHeap, TopLevelAS *pPreviousResult = nullptr) -> TopLevelAS;

    auto GetInstanceCount() const -> size_t {
        return _instances.size();
    }
public:
private:
    // clang-format off
	bool				    _allowUpdate;
	size_t					_scratchSizeInBytes;
	size_t					_resultSizeInBytes;
	std::vector<Instance>   _instances;
    // clang-format on
};

}    // namespace dx
