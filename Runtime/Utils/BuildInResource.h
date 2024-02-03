#pragma once
#include <memory>
#include "GlobalCallbacks.h"
#include "D3d12/D3dStd.h"

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
	auto GetCubeMesh() const -> std::shared_ptr<Mesh> {
		return _pCubeMesh;
	}
	auto GetEmptyLocalRootSignature() const -> const std::shared_ptr<dx::RootSignature> & {
		return _pEmptyLocalRootSignature;
	}
private:
	void BuildSkyBoxCubeMesh();
	void BuildCubeMesh();
private:
	CallbackHandle						_onCreateCallbackHandle;
	CallbackHandle						_onDestroyCallbackHandle;
	std::shared_ptr<Mesh>				_pSkyBoxCubeMesh;
	std::shared_ptr<Mesh>				_pCubeMesh;
	std::shared_ptr<dx::RootSignature>	_pEmptyLocalRootSignature;
};
