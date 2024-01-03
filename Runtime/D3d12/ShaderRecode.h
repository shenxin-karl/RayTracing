#pragma once
#include "D3dStd.h"
#include "Foundation/ReadonlyArraySpan.hpp"

namespace dx {

class LocalRootParameterData {
public:
	LocalRootParameterData(const RootSignature *pRootSignature);
	void SetConstants(size_t rootIndex, ReadonlyArraySpan<DWParam> constants, size_t offset = 0);
	void SetConstants(size_t rootIndex, size_t num32BitValuesToSet, const void *pData, size_t offset = 0);
	void SetView(size_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);
	void SetDescriptorTable(size_t rootIndex, std::shared_ptr<DescriptorHandleArray> pHandleArray);
private:
	enum {
		eConstants,
		eView,
		eDescriptorTable
	};
	auto GetRootParamType(size_t rootIndex) const -> size_t;
	using RootParamData = std::variant<std::vector<DWParam>, D3D12_GPU_VIRTUAL_ADDRESS, std::shared_ptr<DescriptorHandleArray>>;
private:
	const RootSignature			*_pRootSignature;
	std::vector<RootParamData>	 _rootParamDataList;
};

}
