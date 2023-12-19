#include "RenderSetting.h"

auto RenderSetting::Get() -> RenderSetting & {
	static RenderSetting instance;
	return instance;
}

RenderSetting::RenderSetting() {
	_ambientColor = glm::vec3(0.1f);
	_ambientIntensity = 1.f;
}

void RenderSetting::SetAmbientColor(glm::vec3 ambientColor) {
	_ambientColor = ambientColor;
}

auto RenderSetting::GetAmbientColor() const -> glm::vec3 {
	return _ambientColor;
}

void RenderSetting::SetAmbientIntensity(float ambientIntensity) {
	_ambientIntensity = ambientIntensity;
}

auto RenderSetting::GetAmbientIntensity() const -> float {
	return _ambientIntensity;
}
