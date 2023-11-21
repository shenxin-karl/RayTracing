#pragma once

class RenderDoc {
public:
    static bool IsLoaded();
    static bool Load();
    static void Free();
    static void BeginFrameCapture(void *pNativeWindowHandle, void *pDevice);
    static void EndFrameCapture(void *pNativeWindowHandle, void *pDevice);
    static void OpenCaptureInUI();
};