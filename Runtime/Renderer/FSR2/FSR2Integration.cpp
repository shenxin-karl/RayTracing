#include "FSR2Integration.h"
#include <limits>
#include <cmath>

FSR2Integration::FSR2Integration()
    : _scalePreset(),
      _mipBias(0),
      _sharpness(0),
      _jitterIndex(0),
      _useMask(false),
      _RCASSharpen(false),
      _contextDest(),
      _context() {
    SetScalePreset(FSR2ScalePreset::Quality);
}

void FSR2Integration::OnCreate() {
}

void FSR2Integration::OnDestroy() {

}

void FSR2Integration::OnResize(const ResolutionInfo &resolution) {

}

void FSR2Integration::Execute(const FSR2ExecuteDesc &desc) {
}

static float CalculateMipBias(float upscalerRatio) {
    return std::log2f(1.f / upscalerRatio) - 1.f + std::numeric_limits<float>::epsilon();
}

void FSR2Integration::SetScalePreset(FSR2ScalePreset preset) {
    _scalePreset = preset;
    switch (preset) {
    case FSR2ScalePreset::Quality:
        _upscaleRatio = 1.5f;
        break;
    case FSR2ScalePreset::Balanced:
        _upscaleRatio = 1.7f;
        break;
    case FSR2ScalePreset::Performance:
        _upscaleRatio = 2.f;
        break;
    case FSR2ScalePreset::UltraPerformance:
        _upscaleRatio = 3.f;
        break;
    case FSR2ScalePreset::Custom:
    default:;
        break;
    }
    _mipBias = CalculateMipBias(_upscaleRatio);
}

auto FSR2Integration::GetResolutionInfo(size_t width, size_t height) const -> ResolutionInfo {
    ResolutionInfo resolution;
    resolution.renderWidth = static_cast<uint32_t>(std::ceil(static_cast<float>(width) / _upscaleRatio));
    resolution.renderHeight = static_cast<uint32_t>(std::ceil(static_cast<float>(height) / _upscaleRatio));
    resolution.displayWidth = width;
    resolution.displayHeight = height;
    return resolution;
}

void FSR2Integration::GetJitterOffset(const ResolutionInfo &resolution, float &jitterX, float &jitterY) const {
    const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(resolution.renderWidth, resolution.displayWidth);
    ffxFsr2GetJitterOffset(&jitterX, &jitterY, _jitterIndex, jitterPhaseCount);
}
