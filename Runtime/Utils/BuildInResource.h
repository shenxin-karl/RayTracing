#pragma once
#include <memory>
#include "GlobalCallbacks.h"
#include "D3d12/D3dStd.h"
#include "D3d12/DescriptorHandle.h"
#include "D3d12/Texture.h"
#include "Foundation/Memory/SharedPtr.hpp"

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
	auto GetEmptyLocalRootSignature() const -> const SharedPtr<dx::RootSignature> & {
		return _pEmptyLocalRootSignature;
	}
	auto GetWhiteTex() const -> SharedPtr<dx::Texture> {
		return _pWhiteTex;
	}
	auto GetWhiteTexSRV() const -> const dx::SRV & {
		return _whiteTexSRV;
	}
private:
	void BuildSkyBoxCubeMesh();
	void BuildCubeMesh();
	void LoadWhiteTex();
private:
	CallbackHandle						_onCreateCallbackHandle;
	CallbackHandle						_onDestroyCallbackHandle;
	std::shared_ptr<Mesh>				_pSkyBoxCubeMesh;
	std::shared_ptr<Mesh>				_pCubeMesh;
	SharedPtr<dx::Texture>				_pWhiteTex;
	dx::SRV								_whiteTexSRV;
	SharedPtr<dx::RootSignature>		_pEmptyLocalRootSignature;
};
