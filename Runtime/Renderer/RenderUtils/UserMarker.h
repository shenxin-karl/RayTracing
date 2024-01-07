#pragma once
#include <string_view>
#include <d3d12.h>
#include "Foundation/NonCopyable.h"
#include "Foundation/ColorUtil.hpp"

namespace dx {
class Context;
}

class UserMarker : NonCopyable {
public:
	UserMarker(const dx::Context *pContext, std::string_view name);
	UserMarker(ID3D12GraphicsCommandList *pCommandList, std::string_view name);
	~UserMarker();
private:
	ID3D12GraphicsCommandList *_pCommandList = nullptr;
};
