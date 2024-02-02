#pragma once
#include <d3d12.h>
#include "D3d12/D3dStd.h"
#include "Foundation/GlmStd.hpp"
#include "Foundation/NonCopyable.h"
#include <cfloat>

enum class ToneMapperType {
    eAMDToneMapper,
    eDX11SDK,
    eReinhard,
    eUncharted2ToneMap,
    eACESFilm,
    eNone,
};

template<typename T>
struct Var {
public:
    Var(T value, T minValue, T maxValue) : _value(value), _minValue(minValue), _maxValue(maxValue) {
    }
    Var &operator=(T other) {
        _value = std::clamp<T>(other, _minValue, _maxValue);
        return *this;
    }
    operator T() const {
        return _value;
    }
    T *operator&() {
        return &_value;
    }
private:
    T _value;
    T _minValue;
    T _maxValue;
};

enum class ShadowMaskResolution {
    eFull = 0,
    eHalf = 1,
};

struct ShadowConfig {
    Var<float> rayTMin = {0.5f, 0.f, 1.f};
    Var<float> rayTMax = {FLT_MAX, 0.f, FLT_MAX};
    Var<float> sunAngularDiameter = {2.5f, 0.f, 5.f};
    Var<int> rayTraceMaxRecursiveDepth = {1, 1, 5};
    Var<float> denoiseBlurRadiusScale = {2.f, 0.f, 3.f};
    Var<float> planeDistanceSensitivity = {0.005f, 0.f, 0.5f};
    // clang-format on
};

// Enum representing the FSR 2 quality modes.
enum class FSR2ScalePreset {
    Quality = 0,         // 1.5f
    Balanced,            // 1.7f
    Performance,         // 2.f
    UltraPerformance,    // 3.f
    Custom,              // 1.f - 3.f range
};

// Enum representing the reactivity mask modes.
enum class FSR2MaskMode {
    Disabled = 0,
    Manual,
};

struct FSR2Config {
    // clang-format off
	FSR2ScalePreset scalePreset = FSR2ScalePreset::Quality;
    Var<float>      upscaleRatio = {1.5f, 1.f, 3.f};
    FSR2MaskMode    maskMode = FSR2MaskMode::Disabled;
    Var<float>      sharpness = {0.8f, 0.f, 1.f};
    // clang-format on
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
    void SetGamma(float gamma);
    auto GetGamma() -> float;
    void SetReversedZ(bool enable);
    bool GetReversedZ() const;
    auto GetShadowConfig() const -> const ShadowConfig & {
        return _shadowConfig;
    }
    auto GetShadowConfig() -> ShadowConfig & {
        return _shadowConfig;
    }
    auto GetFsr2Config() -> FSR2Config & {
	    return _fsr2Config;
    }
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
	ShadowConfig        _shadowConfig;
    FSR2Config          _fsr2Config;
    // clang-format on
};