#pragma once
#include <memory>
#include "GlobalCallbacks.h"

class Mesh;
class BuildInResource {
public:
	BuildInResource();
	void OnCreate();
	void OnDestroy();
public:
	static auto Get() -> BuildInResource &;
	auto GetSkyBoxCubeMesh() const -> std::shared_ptr<Mesh> {
		return _pSkyBoxCubeMesh;
	}
private:
	void BuildSkyBoxCubeMesh();
private:
	CallbackHandle		  _onCreateCallbackHandle;
	CallbackHandle		  _onDestroyCallbackHandle;
	std::shared_ptr<Mesh> _pSkyBoxCubeMesh;
};
