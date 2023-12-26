#pragma once
#include <functional>
#include <span>
#include <vector>
#include "Foundation/NonCopyable.h"

struct RenderObject;
class RenderPass : private NonCopyable {
public:
	virtual void OnCreate() {}
	virtual void OnDestroy() {}
	virtual ~RenderPass() {}
public:
	using DrawBatchListCallback = std::function<void(std::span<RenderObject *const>)>;
	static void DrawBatchList(const std::vector<RenderObject *> &batchList, const DrawBatchListCallback &callback);
};
