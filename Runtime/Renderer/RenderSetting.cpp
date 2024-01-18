#include "RenderSetting.h"
#include "GfxDevice.h"
#include "D3d12/Device.h"
#include "Utils/GlobalCallbacks.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

auto RenderSetting::Get() -> RenderSetting & {
	static RenderSetting instance;
	return instance;
}

RenderSetting::RenderSetting() {
	_ambientColor = glm::vec3(0.1f);
	_ambientIntensity = 1.f;
	_toneMapperType = ToneMapperType::eAMDToneMapper;
	_exposure = 1.f;
	_gamma = 2.2f;
	_reversedZ = true;
	_shadowRayTMin = 0.1f;
	_shadowRayTMax = 10000.f;
	_shadowSunAngularDiameter = 0.95f;
	_shadowRayTraceMaxRecursiveDepth = 1;
}

void RenderSetting::OnUpdate() {
	ImGui::Begin("RenderSetting");
	{
		ImGui::ColorEdit3("AmbientColor", glm::value_ptr(_ambientColor));
		ImGui::SliderFloat("AmbientIntensity", &_ambientIntensity, 0.1f, 5.f);
		ImGui::LabelText("ReversedZ:", "%s",  _reversedZ ? "Enable" : "Disable");
		GlobalCallbacks::Get().OnBuildRenderSettingGUI.Invoke();
	}
	ImGui::End();
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

void RenderSetting::SetToneMapperType(ToneMapperType toneMapperType) {
	_toneMapperType = toneMapperType;
}

auto RenderSetting::GetToneMapperType() const -> ToneMapperType {
	return _toneMapperType;
}

void RenderSetting::SetExposure(float exposure) {
	_exposure = exposure;
}

auto RenderSetting::GetExposure() const -> float {
	return _exposure;
}

void RenderSetting::SetGamma(float gamma) {
	_gamma = gamma;
}

auto RenderSetting::GetGamma() -> float {
	return _gamma;
}
void RenderSetting::SetReversedZ(bool enable) {
	_reversedZ = enable;
}

bool RenderSetting::GetReversedZ() const {
	return _reversedZ;
}

void RenderSetting::SetShadowRayTMin(float shadowRayTMin) {
	_shadowRayTMin = shadowRayTMin;
}

auto RenderSetting::GetShadowRayTMin() const -> float {
	return _shadowRayTMin;
}

void RenderSetting::SetShadowRayTMax(float shadowRayTMax) {
	_shadowRayTMax = shadowRayTMax;
}

auto RenderSetting::GetShadowRayTMax() const -> float {
	return _shadowRayTMax;
}

void RenderSetting::SetShadowSunAngularDiameter(float angularDiameter) {
	_shadowSunAngularDiameter = angularDiameter;
}

auto RenderSetting::GetShadowSunAngularDiameter() const -> float {
	return _shadowSunAngularDiameter;
}

void RenderSetting::SetShadowRayTraceMaxRecursiveDepth(uint32_t depth) {
	_shadowRayTraceMaxRecursiveDepth = std::max<uint32_t>(depth, 1);
}

auto RenderSetting::GetShadowRayTraceMaxRecursiveDepth() const -> uint32_t {
	return _shadowRayTraceMaxRecursiveDepth;
}

