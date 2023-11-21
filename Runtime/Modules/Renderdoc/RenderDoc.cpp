#include "RenderDoc.h"
#include "Foundation/NamespeceAlias.h"
#include <app/renderdoc_app.h>
#include <Windows.h>
#include "Foundation/Exception.h"

#if PLATFORM_WIN
    #include "Foundation/Platform/Win/RegistryUtils.h"
#endif

static RENDERDOC_API_1_0_0 *sRenderDocApi = nullptr;
[[maybe_unused]]
static HINSTANCE sRenderDocDllModule = nullptr;

bool RenderDoc::IsLoaded() {
    return sRenderDocApi != nullptr;
}

bool RenderDoc::Load() {
#if ENABLE_RENDER_DOC
    if (IsLoaded()) {
        Exception::Throw("RenderDoc Is Loaded!");
        return true;
    }

    auto InitRenderDocApi = [](const stdfs::path &modulePath) {
        if (sRenderDocDllModule != nullptr) {
            return;
        }

        sRenderDocDllModule = LoadLibrary(modulePath.wstring().c_str());
        if (sRenderDocDllModule == nullptr) {
            return;
        }
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(
            GetProcAddress(sRenderDocDllModule, "RENDERDOC_GetAPI"));
        if (RENDERDOC_GetAPI) {
            if (RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void **>(&sRenderDocApi))) {
                sRenderDocApi->MaskOverlayBits(0, 0);
                sRenderDocApi->SetFocusToggleKeys(nullptr, 0);
                sRenderDocApi->SetCaptureKeys(nullptr, 0);
                sRenderDocApi->UnloadCrashHandler();
            }
        }
    };

    #if PLATFORM_WIN
    std::wstring_view path = L"Software\\Classes\\CLSID\\{5D6BF029-A6BA-417A-8523-120492B1DCE3}\\InprocServer32";
    if (std::optional<std::string> pDllPath = Registry::GetString(HKEY_LOCAL_MACHINE, path.data(), L"")) {
        InitRenderDocApi(*pDllPath);
    }
    #endif

    InitRenderDocApi("renderdoc.dll");
    return sRenderDocApi != nullptr;
#endif
    return false;
}

void RenderDoc::Free() {
#if ENABLE_RENDER_DOC
    if (sRenderDocDllModule) {
        FreeModule(sRenderDocDllModule);
        sRenderDocDllModule = nullptr;
    }
#endif
}

void RenderDoc::BeginFrameCapture(void *pNativeWindowHandle, void *pDevice) {
#if ENABLE_RENDER_DOC
    if (sRenderDocApi == nullptr) {
        return;
    }
    sRenderDocApi->StartFrameCapture(pDevice, pNativeWindowHandle);
#endif
}

void RenderDoc::EndFrameCapture(void *pNativeWindowHandle, void *pDevice) {
#if ENABLE_RENDER_DOC
    if (sRenderDocApi == nullptr) {
        return;
    }
    sRenderDocApi->EndFrameCapture(pDevice, pNativeWindowHandle);
#endif
}

void RenderDoc::OpenCaptureInUI() {
#if ENABLE_RENDER_DOC
    if (!sRenderDocApi->IsRemoteAccessConnected()) {
        sRenderDocApi->LaunchReplayUI(true, "");
    }
#endif
}
