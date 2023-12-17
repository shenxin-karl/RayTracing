#pragma once
#include <vector>
#include "Foundation/NonCopyable.h"
#include "Foundation/GlmStd.hpp"

class RenderObjectKey {
	uint64_t key1;
	uint64_t key2;
public:
	RenderObjectKey();
	void SetMaterialID(uint16_t materialID);
	auto GetMaterialID() const -> uint16_t;
	void SetPipelineID(uint16_t variantID);
	auto GetPipelineID() const -> uint16_t;
	void SetRenderGroup(uint16_t renderGroup);
	auto GetRenderGroup() const -> uint16_t;
	void SetDepthSquare(double depthSqr);
	void SetPriority(uint16_t priority);
	auto GetPriority() const -> uint16_t;
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
