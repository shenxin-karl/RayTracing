#include "Pix.h"
#define USE_PIX
#include <Windows.h>
#include <WinPixEventRuntime/pix3.h>
#include "D3d12/D3dUtils.h"
#include "Utils/AssetProjectSetting.h"

static HMODULE sModule = nullptr;
bool Pix::IsLoaded() {
    return sModule != nullptr;
}

bool Pix::Load() {
    Assert(sModule == nullptr);
    sModule = PIXLoadLatestWinPixGpuCapturerLibrary();

    stdfs::path path = AssetProjectSetting::ToCachePath("CaptureFrame");
    if (!stdfs::exists(path)) {
        stdfs::create_directories(path);
    }
    return sModule != nullptr;
}

void Pix::Free() {
    Assert(sModule != nullptr);
    FreeModule(sModule);
    sModule = nullptr;
}

void Pix::BeginFrameCapture(void *pNativeWindowHandle, void *pDevice) {
    if (PIXGetCaptureState() == PIX_CAPTURE_GPU) {
	    return;
    }

    dx::ThrowIfFailed(PIXSetTargetWindow(static_cast<HWND>(pNativeWindowHandle)));
    PIXCaptureParameters captureParameters = {};
    std::wstring path = AssetProjectSetting::ToCachePath("CaptureFrame/PixCapture.wpix").wstring();
    captureParameters.GpuCaptureParameters.FileName = path.c_str();
    dx::ThrowIfFailed(PIXBeginCapture(PIX_CAPTURE_GPU, &captureParameters));
}

void Pix::EndFrameCapture(void *pNativeWindowHandle, void *pDevice) {
    dx::ThrowIfFailed(PIXEndCapture(false));
    MainThread::AddMainThreadJob(MainThread::PostRender, []() {
	    if (PIXGetCaptureState() == PIX_CAPTURE_GPU) {
		    return MainThread::ExecuteInNextFrame;
	    }

	    std::wstring path = AssetProjectSetting::ToCachePath("CaptureFrame/PixCapture.wpix").wstring();
	    PIXOpenCaptureInUI(path.c_str());
	    return MainThread::Finished;
    });
}
