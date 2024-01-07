#pragma once
#include "Foundation/ITick.hpp"
#include "Foundation/Singleton.hpp"
#include <Windows.h>
#include <dxgiformat.h>

namespace dx {

class Device;
class SwapChain;
class FrameResourceRing;
class UploadHeap;
class SyncASBuilder;
class DescriptorManager;
}

class GfxDevice : public Singleton<GfxDevice> {
public:
	GfxDevice();
	~GfxDevice() override;
public:
	void OnCreate(uint32_t numBackBuffer, HWND hwnd, DXGI_FORMAT rtFormat, DXGI_FORMAT dsFormat);
	void OnDestroy();
	auto GetNumBackBuffer() const -> size_t {
		return _numBackBuffer;
	}
	auto GetDevice() const -> dx::Device * {
		return _pDevice.get();
	}
	auto GetSwapChain() const -> dx::SwapChain * {
		return _pSwapChain.get();
	}
	auto GetUploadHeap() const -> dx::UploadHeap * {
		return _pUploadHeap.get();
	}
	auto GetASBuilder() const -> dx::SyncASBuilder * {
		return _pASBuilder.get();
	}
	auto GetRenderTargetFormat() const-> DXGI_FORMAT {
		return _renderTargetFormat;
	}
	auto GetDepthStencilFormat() const -> DXGI_FORMAT {
		return _depthStencilFormat;
	}
private:
	// clang-format off
	size_t								_numBackBuffer;
	std::unique_ptr<dx::Device>			_pDevice;
	std::unique_ptr<dx::SwapChain>		_pSwapChain;
	std::unique_ptr<dx::UploadHeap>		_pUploadHeap;
	std::unique_ptr<dx::SyncASBuilder>	_pASBuilder;
	DXGI_FORMAT							_renderTargetFormat;
	DXGI_FORMAT							_depthStencilFormat;
	// clang-format on
};