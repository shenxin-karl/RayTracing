#pragma once
#include <vector>
#include "Foundation/NonCopyable.h"
#include "Foundation/GlmStd.hpp"

class RenderObjectKey {
	union {
		uint64_t key1;
		struct {
			uint32_t pipelineID;
			uint16_t priority;
			uint16_t renderGroup;
		};
	};
	union {
		uint64_t key2;
		double distanceSqr;
	};
public:
	RenderObjectKey();
	void SetRenderGroup(uint16_t renderGroup);
	auto GetRenderGroup() const -> uint16_t;
	void SetPriority(uint16_t priority);
	auto GetPriority() const -> uint16_t;
	void SetPipelineID(uint32_t variantID);
	auto GetPipelineID() const -> uint32_t;
	void SetDepthSquare(double depthSqr);
	auto GetDepthSquare() const -> double;
	friend std::strong_ordering operator<=>(const RenderObjectKey &, const RenderObjectKey &);
};

struct RenderObject;
class SceneRenderObjectManager : private NonCopyable {
public:
	void AddRenderObject(RenderObject *pRenderObject) {
		_renderObjects.push_back(pRenderObject);
	}
	void Reset() {
		_renderObjects.clear();
	}
	auto GetOpaqueRenderObjects() const -> const std::vector<RenderObject *> & {
		return _opaqueRenderObjects;
	}
	auto GetAlphaTestRenderObjects() const -> const std::vector<RenderObject *> & {
		return _alphaTestRenderObjects;
	}
	auto GetTransparentRenderObjects() const -> const std::vector<RenderObject *> & {
		return _transparentRenderObjects;
	}
	void ClassifyRenderObjects(const glm::vec3 &worldCameraPos);
private:
	std::vector<RenderObject *> _renderObjects;
	std::vector<RenderObject *>	_opaqueRenderObjects;
	std::vector<RenderObject *> _alphaTestRenderObjects;
	std::vector<RenderObject *>	_transparentRenderObjects;
};
