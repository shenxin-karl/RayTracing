#pragma once
#include <string_view>
#include "Foundation/NonCopyable.h"
#include "Foundation/ColorUtil.hpp"

namespace dx {
class Context;
}

class UserMarker : NonCopyable {
public:
	UserMarker(const dx::Context *pContext, std::string_view name);
	~UserMarker();
private:
	const dx::Context *_pContext = nullptr;
};
