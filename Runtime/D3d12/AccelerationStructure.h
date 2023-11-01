#pragma once
#include "D3dUtils.h"

namespace dx {

class AccelerationStructure : public NonCopyable {
public:
	AccelerationStructure();
	~AccelerationStructure();
protected:
	WRL::ComPtr<D3D12MA::Allocation> _pAllocation;
};

class BottomLevelAS : public AccelerationStructure {
};

class TopLevelAS : public AccelerationStructure {
};

}
