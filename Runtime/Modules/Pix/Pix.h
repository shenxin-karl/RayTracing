#pragma once

namespace dx {
class Device;
}

class Pix {
public:
	static bool IsLoaded();
    static bool Load();
    static void Free();
	static void BeginFrameCapture(void *pNativeWindowHandle, dx::Device *pDevice);
	static void EndFrameCapture(void *pNativeWindowHandle, dx::Device *pDevice);
	static void OpenCaptureInUI();
};
