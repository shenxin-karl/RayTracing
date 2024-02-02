#pragma once
#include <functional>
#include <span>
#include <vector>
#include "Foundation/NonCopyable.h"
#include "Renderer/RenderUtils/ResolutionInfo.hpp"

struct RenderObject;
class RenderPass : private NonCopyable {
public:
	virtual void OnDestroy() {}
	virtual ~RenderPass() {}
	virtual void OnResize(const ResolutionInfo &resolution) {}
public:
	using DrawBatchListCallback = std::function<void(std::span<RenderObject *const>)>;
	static void DrawBatchList(const std::vector<RenderObject *> &batchList, const DrawBatchListCallback &callback);
};
