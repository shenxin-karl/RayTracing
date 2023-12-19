#pragma once
#include "Foundation/GlmStd.hpp"
#include "Foundation/NonCopyable.h"

class RenderSetting final : NonCopyable {
public:
	static auto Get() -> RenderSetting &;
	RenderSetting();
public:
	void SetAmbientColor(glm::vec3 ambientColor);
	auto GetAmbientColor() const -> glm::vec3;
	void SetAmbientIntensity(float ambientIntensity);
	auto GetAmbientIntensity() const -> float;
private:
	// clang-format off
	glm::vec3 _ambientColor;
	float	  _ambientIntensity;
	// clang-format on
};