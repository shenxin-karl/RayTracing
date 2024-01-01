#include "RenderSetting.h"

#include "GfxDevice.h"
#include "D3d12/Device.h"
#include "Utils/GlobalCallbacks.h"

auto RenderSetting::Get() -> RenderSetting & {
	static RenderSetting instance;
	return instance;
}

RenderSetting::RenderSetting() {
	_ambientColor = glm::vec3(0.1f);
	_ambientIntensity = 1.f;
	_toneMapperType = ToneMapperType::eAMDToneMapper;
	_exposure = 1.f;
	_reversedZ = true;

	_needRecreatePipeline = false;
}

void RenderSetting::OnPreRender() {
	bool waitGpuFlush = (_needRecreatePipeline);
	if (waitGpuFlush) {
		GfxDevice::GetInstance()->GetDevice()->WaitForGPUFlush();
	}
	if (_needRecreatePipeline) {
		GlobalCallbacks::Get().onRecreatePipelineState.Invoke();
		_needRecreatePipeline = false;
	}
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

void RenderSetting::SetReversedZ(bool enable) {
	_needRecreatePipeline |= (_reversedZ != enable);
	_reversedZ = enable;
}

bool RenderSetting::GetReversedZ() const {
	return _reversedZ;
}


