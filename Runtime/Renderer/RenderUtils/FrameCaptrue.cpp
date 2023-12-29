#include "FrameCaptrue.h"
#include "D3d12/Device.h"
#include "Modules/Renderdoc/RenderDoc.h"
#include "Modules/Pix/Pix.h"

#if 1
using NativeFrameCaptureType = Pix;
#else
using NativeFrameCaptureType = RenderDoc;
#endif

bool FrameCapture::IsLoaded() {
    return NativeFrameCaptureType::IsLoaded();
}

bool FrameCapture::Load() {
    bool success = NativeFrameCaptureType::Load();
    Assert(success);
    return success;
}

void FrameCapture::Free() {
    return NativeFrameCaptureType::Free();
}

void FrameCapture::BeginFrameCapture(void *pNativeWindowHandle, dx::Device *pDevice) {
    pDevice->WaitForGPUFlush();
    NativeFrameCaptureType::BeginFrameCapture(pNativeWindowHandle, pDevice);
}

void FrameCapture::EndFrameCapture(void *pNativeWindowHandle, dx::Device *pDevice) {
    pDevice->WaitForGPUFlush();
    NativeFrameCaptureType::EndFrameCapture(pNativeWindowHandle, pDevice);
}

void FrameCapture::OpenCaptureInUI() {
    NativeFrameCaptureType::OpenCaptureInUI();
}
