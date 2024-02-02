#pragma once
#include <cstdint>
#include <cmath>

struct ResolutionInfo {
    uint16_t renderWidth;
    uint16_t renderHeight;
    uint16_t displayWidth;
    uint16_t displayHeight;
    float jitterX;
    float jitterY;
public:
    ResolutionInfo() = default;

    ResolutionInfo(size_t displayWidth, size_t displayHeight)
        : renderWidth(displayWidth),
          renderHeight(displayHeight),
          displayWidth(displayWidth),
          displayHeight(displayHeight),
          jitterX(0.f),
          jitterY(0.f) {
    }

    ResolutionInfo(size_t displayWidth, size_t displayHeight, float upscaleRatio, float jitterX, float jitterY)
        : renderWidth(static_cast<uint16_t>(std::ceil(displayWidth / upscaleRatio))),
          renderHeight(static_cast<uint16_t>(std::ceil(displayHeight / upscaleRatio))),
          displayWidth(displayWidth),
          displayHeight(displayHeight),
          jitterX(jitterX),
          jitterY(jitterY) {
    }
};