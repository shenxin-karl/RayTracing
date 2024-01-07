#pragma once
#include <d3d12.h>

#include "D3d12/D3dStd.h"
#include "Foundation/GlmStd.hpp"
#include "Foundation/NonCopyable.h"

enum class ToneMapperType {
    eAMDToneMapper,
    eDX11SDK,
    eReinhard,
    eUncharted2ToneMap,
    eACESFilm,
    eNone,
};

class RenderSetting final : NonCopyable {
public:
    static auto Get() -> RenderSetting &;
    RenderSetting();
public:
    void OnUpdate();
    void SetAmbientColor(glm::vec3 ambientColor);
    auto GetAmbientColor() const -> glm::vec3;
    void SetAmbientIntensity(float ambientIntensity);
    auto GetAmbientIntensity() const -> float;
    void SetToneMapperType(ToneMapperType toneMapperType);
    auto GetToneMapperType() const -> ToneMapperType;
    void SetExposure(float exposure);
    auto GetExposure() const -> float;
    void SetReversedZ(bool enable);
    bool GetReversedZ() const;
    void SetShadowRayTMin(float shadowRayTMin);
    auto GetShadowRayTMin() const -> float;
    void SetShadowRayTMax(float shadowRayTMax);
    auto GetShadowRayTMax() const -> float;
    void SetShadowRayMaxCosineTheta(float rayMaxCosineTheta);
    auto GetShadowRayMaxCosineTheta() const -> float;
    void SetGamma(float gamma);
    auto GetGamma() -> float;
public:
    auto GetDepthClearValue() const -> float {
        return _reversedZ ? 0.f : 1.f;
    }
    auto GetDepthFunc() const -> D3D12_COMPARISON_FUNC {
        return _reversedZ ? D3D12_COMPARISON_FUNC_GREATER : D3D12_COMPARISON_FUNC_LESS;
    }


private:
    // clang-format off
	glm::vec3			_ambientColor;
	float				_ambientIntensity;
	ToneMapperType		_toneMapperType;
	float				_exposure;
    float               _gamma;
	bool				_reversedZ;
    float               _shadowRayTMin;
    float               _shadowRayTMax;
    float               _shadowRayMaxCosineTheta;
    // clang-format on
};