#include "Pix.h"
#define USE_PIX
#include <Windows.h>
#include <WinPixEventRuntime/pix3.h>
#include "D3d12/D3dUtils.h"
#include "D3d12/Device.h"
#include "Utils/AssetProjectSetting.h"

static size_t sIndex = 1;
static HMODULE sModule = nullptr;

static std::string GetCaptureFileName() {
	return fmt::format("CaptureFrame/PixCapture_{}.wpix", sIndex);
}

bool Pix::IsLoaded() {
    return sModule != nullptr;
}

bool Pix::Load() {
    Assert(sModule == nullptr);
    sModule = PIXLoadLatestWinPixGpuCapturerLibrary();

    stdfs::path path = AssetProjectSetting::ToCachePath("CaptureFrame");
    if (!stdfs::exists(path)) {
        stdfs::remove_all(path);
    }
    return sModule != nullptr;
}

void Pix::Free() {
    Assert(sModule != nullptr);
    FreeModule(sModule);
    sModule = nullptr;
}

void Pix::BeginFrameCapture(void *pNativeWindowHandle, dx::Device *pDevice) {
    pDevice->WaitForGPUFlush();
    dx::ThrowIfFailed(PIXSetTargetWindow(static_cast<HWND>(pNativeWindowHandle)));

    PIXCaptureParameters captureParameters = {};
    std::wstring path = AssetProjectSetting::ToCachePath(GetCaptureFileName()).wstring();

    std::error_code errorCode = {};
	stdfs::remove(path, errorCode);

    captureParameters.GpuCaptureParameters.FileName = path.c_str();
    dx::ThrowIfFailed(PIXBeginCapture(PIX_CAPTURE_GPU, &captureParameters));
}

void Pix::EndFrameCapture(void *pNativeWindowHandle, dx::Device *pDevice) {
    pDevice->WaitForGPUFlush();
    dx::ThrowIfFailed(PIXEndCapture(false));
}

void Pix::OpenCaptureInUI() {
    stdfs::path path = AssetProjectSetting::ToCachePath(GetCaptureFileName());
    PIXOpenCaptureInUI(path.c_str());
    ++sIndex;
}
