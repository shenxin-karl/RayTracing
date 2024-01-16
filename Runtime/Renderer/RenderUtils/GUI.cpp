#include "GUI.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "UserMarker.h"
#include "D3d12/Device.h"
#include "D3d12/ResourceStateTracker.h"
#include "D3d12/SwapChain.h"
#include "Renderer/GfxDevice.h"
#include <ShellScalingApi.h>

void GUI::OnCreate() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;     // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    //ImGui::StyleColorsClassic();

    if (!InitD3DObjects()) {
        return;
    }

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();
    ImGui_ImplWin32_Init(pGfxDevice->GetSwapChain()->GetHWND());
    ImGui_ImplDX12_Init(device,
        pGfxDevice->GetNumBackBuffer(),
        pGfxDevice->GetSwapChain()->GetFormat(),
        _pSrvDescHeap.Get(),
        _pSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        _pSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    float fontScale = ImGui_ImplWin32_GetDpiScaleForHwnd(pGfxDevice->GetSwapChain()->GetHWND());
    UpdateUIScaling(fontScale);
}

void GUI::OnDestroy() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    pGfxDevice->GetDevice()->WaitForGPUFlush();

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    _pGraphicsQueue = nullptr;
    _pSrvDescHeap = nullptr;
    _frameContexts.clear();
    _pCommandList = nullptr;
    _pFence->OnDestroy();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool GUI::PollEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
        return true;
    }

    switch (msg) {
    case WM_DPICHANGED: {
        RECT *rect = reinterpret_cast<RECT *>(lParam);
        IM_ASSERT(LOWORD(wParam) == HIWORD(wParam));
        SetWindowPos(hwnd,
            NULL,
            rect->left,
            rect->top,
            rect->right - rect->left,
            rect->bottom - rect->top,
            SWP_NOZORDER);
        UpdateUIScaling(static_cast<float>(LOWORD(wParam)) / static_cast<float>(USER_DEFAULT_SCREEN_DPI));
        break;
    }
    default:
        break;
    }
    return false;
}

void GUI::NewFrame() {
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void GUI::Render() {

    ImGui::Render();
    UINT nextFrameIndex = _frameIndex + 1;
    _frameIndex = nextFrameIndex;

    size_t numContexts = _frameContexts.size();
    FrameContext *pFrameCtx = &_frameContexts[nextFrameIndex % numContexts];
    _pFence->CpuWaitForFence(pFrameCtx->fenceValue);

    dx::ThrowIfFailed(pFrameCtx->pCommandAllocator->Reset());
    dx::ThrowIfFailed(_pCommandList->Reset(pFrameCtx->pCommandAllocator.Get(), nullptr));
    {
        UserMarker userMarker(_pCommandList.Get(), "GUI::Render");

        dx::SwapChain *pSwapChain = GfxDevice::GetInstance()->GetSwapChain();
        ID3D12Resource *pRenderTarget = pSwapChain->GetCurrentBackBuffer();
        dx::GlobalResourceState::Lock();
        dx::GlobalResourceState::ResourceState *pResourceState = dx::GlobalResourceState::FindResourceState(
            pRenderTarget);
        dx::GlobalResourceState::UnLock();

        // Before rendering the GUI, we need to switch SwapChain::RenderTarget to the D3D12_RESOURCE_STATE_PRESENT state
        Assert(pResourceState->state == D3D12_RESOURCE_STATE_PRESENT);
        Assert(pResourceState->subResourceStateMap.empty());

        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pRenderTarget,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        _pCommandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = pSwapChain->GetCurrentBackBufferRTV();
        _pCommandList->OMSetRenderTargets(1, &rtv, false, nullptr);
        _pCommandList->SetDescriptorHeaps(1, _pSrvDescHeap.GetAddressOf());
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _pCommandList.Get());

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(pRenderTarget,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
        _pCommandList->ResourceBarrier(1, &barrier);
    }
    dx::ThrowIfFailed(_pCommandList->Close());
    ID3D12CommandList *cmdList[] = {_pCommandList.Get()};
    _pGraphicsQueue->ExecuteCommandLists(1, cmdList);

    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // Update and Render additional Platform Windows
    //if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    //{
    //    ImGui::UpdatePlatformWindows();
    //    ImGui::RenderPlatformWindowsDefault(nullptr, (void*)g_pd3dCommandList);
    //}
    pFrameCtx->fenceValue = _pFence->IssueFence(_pGraphicsQueue);
}

auto GUI::Get() -> GUI & {
    static GUI instance;
    return instance;
}

bool GUI::InitD3DObjects() {
    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::NativeDevice *device = pGfxDevice->GetDevice()->GetNativeDevice();

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_pSrvDescHeap)) != S_OK) {
        Assert(false);
        return false;
    }

    _frameContexts.resize(pGfxDevice->GetNumBackBuffer());
    for (UINT i = 0; i < pGfxDevice->GetNumBackBuffer(); i++) {
        if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&_frameContexts[i].pCommandAllocator)) != S_OK) {
            Assert(false);
            return false;
        }
    }

    if (device->CreateCommandList(0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            _frameContexts[0].pCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&_pCommandList)) != S_OK ||
        _pCommandList->Close() != S_OK) {

        Assert(false);
        return false;
    }

    _pGraphicsQueue = pGfxDevice->GetDevice()->GetGraphicsQueue();

    _pFence = std::make_unique<dx::Fence>();
    _pFence->OnCreate(pGfxDevice->GetDevice(), "GUI::Fence");
    return true;
}

void GUI::UpdateUIScaling(float scale) {
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplDX12_InvalidateDeviceObjects();

    // Setup Dear ImGui style
    ImGuiStyle &style = ImGui::GetStyle();
    ImGuiStyle styleold = style;    // Backup colors
    style =
        ImGuiStyle();    // IMPORTANT: ScaleAllSizes will change the original size, so we should reset all style config
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.ScaleAllSizes(scale);
    CopyMemory(style.Colors, styleold.Colors, sizeof(style.Colors));    // Restore colors

    io.Fonts->Clear();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    int pixSize = 14.0f * scale;
    ImFont *font = io.Fonts->AddFontFromFileTTF("C:\\WINDOWS\\Fonts\\consola.ttf",
        pixSize,
        nullptr,
        io.Fonts->GetGlyphRangesJapanese());
    Assert(font != nullptr);
}
