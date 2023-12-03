#pragma once
#include "Foundation/ITick.hpp"
#include "Foundation/Singleton.hpp"
#include <Windows.h>

namespace dx {

class Device;
class SwapChain;
class FrameResourceRing;
class UploadHeap;
class ASBuilder;
class DescriptorManager;
}

class GfxDevice : public Singleton<GfxDevice> {
public:
	GfxDevice();
	~GfxDevice() override;
public:
	void OnCreate(uint32_t numBackBuffer, HWND hwnd);
	void OnDestroy();
	auto GetDevice() const -> dx::Device * {
		return _pDevice.get();
	}
	auto GetSwapChain() const -> dx::SwapChain * {
		return _pSwapChain.get();
	}
	auto GetUploadHeap() const -> dx::UploadHeap * {
		return _pUploadHeap.get();
	}
	auto GetASBuilder() const -> dx::ASBuilder * {
		return _pASBuilder.get();
	}
private:
	// clang-format off
	size_t							_numBackBuffer;
	std::unique_ptr<dx::Device>		_pDevice;
	std::unique_ptr<dx::SwapChain>	_pSwapChain;
	std::unique_ptr<dx::UploadHeap>	_pUploadHeap;
	std::unique_ptr<dx::ASBuilder>	_pASBuilder;
	// clang-format on
};