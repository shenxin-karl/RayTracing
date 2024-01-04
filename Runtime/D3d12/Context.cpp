#include "Context.h"
#include "DescriptorHandleArray.hpp"

namespace dx {

void ComputeContext::DispatchRays(const DispatchRaysDesc &dispatchRaysDesc) {
#if ENABLE_RAY_TRACING
    std::unordered_map<DescriptorHandleArray *, D3D12_GPU_DESCRIPTOR_HANDLE> gpuDescriptorHandleMap;
    size_t handleCount = 0;

    auto PrepareDescriptorHandleArray = [&](ReadonlyArraySpan<ShaderRecode> shaderRecodeList) {
        for (auto &shaderRecode : shaderRecodeList) {
            const LocalRootParameterData &localRootParameterData = shaderRecode.GetLocalRootParameterData();
            for (size_t i = 0; i < localRootParameterData._rootParamDataList.size(); ++i) {
                auto &rootParamData = localRootParameterData._rootParamDataList[i];
                Assert(rootParamData.index() != std::variant_npos);
                if (rootParamData.index() == LocalRootParameterData::eDescriptorTable) {
                    DescriptorHandleArray *pDescriptorHandleArray = std::get<2>(rootParamData).get();
                    if (!gpuDescriptorHandleMap.contains(pDescriptorHandleArray)) {
                        handleCount += pDescriptorHandleArray->Count();
                        gpuDescriptorHandleMap[pDescriptorHandleArray] = {0};
                    }
                }
            }
        }
    };

    auto AllocDescriptorTableHandle = [&]() {
        size_t staleCount = _dynamicViewDescriptorHeap.ComputeStaleDescriptorCount();
        _dynamicViewDescriptorHeap.EnsureCapacity(_pCommandList, staleCount + handleCount);
        for (auto &&[pDescriptorHandleArray, baseHandle] : gpuDescriptorHandleMap) {
            baseHandle = _dynamicViewDescriptorHeap.CommitDescriptorHandleArray(pDescriptorHandleArray);
        }
    };

    auto SerializeShaderRecode = [&](uint8_t *pDest, const ShaderRecode &shaderRecode) {
        std::memcpy(pDest, shaderRecode.GetShaderIdentifier(), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        pDest += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

        const LocalRootParameterData &localRootParameterData = shaderRecode.GetLocalRootParameterData();
        if (localRootParameterData.GetSize() == 0) {
            return;
        }

        for (size_t i = 0; i < localRootParameterData._rootParamDataList.size(); ++i) {
            auto &rootParamData = localRootParameterData._rootParamDataList[i];
            Assert(rootParamData.index() != std::variant_npos);
            switch (rootParamData.index()) {
            case LocalRootParameterData::eConstants:
                pDest = reinterpret_cast<uint8_t *>(AlignUp(reinterpret_cast<intptr_t>(pDest), sizeof(DWParam)));
                for (auto &dwParam : std::get<0>(rootParamData)) {
                    std::memcpy(pDest, &dwParam, sizeof(DWParam));
                    pDest += sizeof(DWParam);
                }
                break;
            case LocalRootParameterData::eView:
                pDest = reinterpret_cast<uint8_t *>(
                    AlignUp(reinterpret_cast<intptr_t>(pDest), sizeof(D3D12_GPU_VIRTUAL_ADDRESS)));
                std::memcpy(pDest, &std::get<1>(rootParamData), sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
                pDest += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
                break;
            case LocalRootParameterData::eDescriptorTable:
                pDest = reinterpret_cast<uint8_t *>(
                    AlignUp(reinterpret_cast<intptr_t>(pDest), sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)));
                DescriptorHandleArray *pDescriptorHandleArray = std::get<2>(rootParamData).get();
                D3D12_GPU_DESCRIPTOR_HANDLE baseHandle = gpuDescriptorHandleMap.find(pDescriptorHandleArray)->second;
                std::memcpy(pDest, &baseHandle, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
                pDest += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
                break;
            default:
                Assert(false);
            };
        }
    };

    auto SerializeShaderRecodeTable =
        [&](ReadonlyArraySpan<ShaderRecode> shaderRecodeList) -> D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE {
        if (shaderRecodeList.Empty()) {
            return {};
        }

        size_t stride = 0;
        for (auto &shaderRecode : shaderRecodeList) {
            stride = std::max(stride, shaderRecode.GetSize());
        }

        size_t bufferSize = shaderRecodeList.Count() * stride;
        DynamicBufferAllocator::AllocInfo allocInfo = _dynamicBufferAllocator.AllocBuffer(bufferSize,
            ShaderRecode::kAddressAlignment);
        uint8_t *pDest = allocInfo.pBuffer;
        std::memset(pDest, 0, bufferSize);
        for (const ShaderRecode &shaderRecode : shaderRecodeList) {
            SerializeShaderRecode(pDest, shaderRecode);
            pDest += stride;
        }

        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE ret = {};
        ret.StartAddress = allocInfo.virtualAddress;
        ret.SizeInBytes = bufferSize;
        ret.StrideInBytes = stride;
        return ret;
    };

    PrepareDescriptorHandleArray(dispatchRaysDesc.rayGenerationShaderRecode);
    PrepareDescriptorHandleArray(dispatchRaysDesc.missShaderTable);
    PrepareDescriptorHandleArray(dispatchRaysDesc.hitGroupTable);

    AllocDescriptorTableHandle();

    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.Width = dispatchRaysDesc.width;
    desc.Height = dispatchRaysDesc.height;
    desc.Depth = dispatchRaysDesc.depth;

    size_t rayGenerationSize = dispatchRaysDesc.rayGenerationShaderRecode.GetSize();
    DynamicBufferAllocator::AllocInfo rayGenerationShaderRecodeAllocInfo = _dynamicBufferAllocator.AllocBuffer(
        rayGenerationSize,
        ShaderRecode::kAddressAlignment);
    SerializeShaderRecode(rayGenerationShaderRecodeAllocInfo.pBuffer, dispatchRaysDesc.rayGenerationShaderRecode);
    desc.RayGenerationShaderRecord.StartAddress = rayGenerationShaderRecodeAllocInfo.virtualAddress;
    desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSize;

    desc.MissShaderTable = SerializeShaderRecodeTable(dispatchRaysDesc.missShaderTable);
    desc.HitGroupTable = SerializeShaderRecodeTable(dispatchRaysDesc.hitGroupTable);
    desc.CallableShaderTable = SerializeShaderRecodeTable(dispatchRaysDesc.callShaderTable);

    FlushResourceBarriers();
    _dynamicViewDescriptorHeap.CommitStagedDescriptorForDispatch(_pCommandList);
    _dynamicSampleDescriptorHeap.CommitStagedDescriptorForDispatch(_pCommandList);
    _pCommandList->DispatchRays(&desc);
#endif
}

}    // namespace dx
