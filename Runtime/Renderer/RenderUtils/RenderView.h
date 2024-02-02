#pragma once
#include "ResolutionInfo.hpp"
#include "Renderer/RenderUtils/ConstantBufferHelper.h"

class DirectionalLight;

class RenderView {
public:
    RenderView();
    void Step0_OnNewFrame();
    void Step1_UpdateCameraMatrix(const Camera *pCamera);
    void Step2_UpdateResolutionInfo(const ResolutionInfo &resolution);
    void Step3_UpdateDirectionalLightInfo(const DirectionalLight *pDirectionalLight);
    void BeforeFinalizeOptional_SetMipBias(float mipBias);
    void Step4_Finalize();
public:
    auto GetCBPrePass() const -> const cbuffer::CbPrePass & {
        return _cbPrePass;
    }
    auto GetCBLighting() const -> const cbuffer::CbLighting & {
        return _cbLighting;
    }
    auto GetRenderSizeViewport() const -> D3D12_VIEWPORT {
        return D3D12_VIEWPORT{
            0.f,
            0.f,
            _cbPrePass.renderSize.x,
            _cbPrePass.renderSize.y,
            0.f,
            1.f,
        };
    }
    auto GetDisplaySizeViewport() const -> D3D12_VIEWPORT {
        return D3D12_VIEWPORT{
            0.f,
            0.f,
            _cbPrePass.displaySize.x,
            _cbPrePass.displaySize.y,
            0.f,
            1.f,
        };
    }
    auto GetRenderSizeScissorRect() const -> D3D12_RECT {
        return D3D12_RECT{
            0,
            0,
            static_cast<LONG>(_cbPrePass.renderSize.x),
            static_cast<LONG>(_cbPrePass.renderSize.y),
        };
    }
    auto GetDisplaySizeScissorRect() const -> D3D12_RECT {
        return D3D12_RECT{
            0,
            0,
            static_cast<LONG>(_cbPrePass.displaySize.x),
            static_cast<LONG>(_cbPrePass.displaySize.y),
        };
    }
private:
    void UpdatePreviousFrameData();
private:
    // clang-format off
    bool                _isFirstFrame;
	cbuffer::CbPrePass  _cbPrePass;
	cbuffer::CbLighting _cbLighting;
    // clang-format on
};