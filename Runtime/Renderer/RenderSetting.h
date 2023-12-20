#pragma once
#include "Foundation/GlmStd.hpp"
#include "Foundation/NonCopyable.h"

enum class ToneMapperType {
	eAMDToneMapper,
	eDX11SDK,
	eReinhard,
	eUncharted2ToneMap,
	eACESFilm
};

class RenderSetting final : NonCopyable {
public:
	static auto Get() -> RenderSetting &;
	RenderSetting();
public:
	void SetAmbientColor(glm::vec3 ambientColor);
	auto GetAmbientColor() const -> glm::vec3;
	void SetAmbientIntensity(float ambientIntensity);
	auto GetAmbientIntensity() const -> float;
	void SetToneMapperType(ToneMapperType toneMapperType);
	auto GetToneMapperType() const -> ToneMapperType;
	void SetExposure(float exposure);
	auto GetExposure() const -> float;
private:
	// clang-format off
	glm::vec3			_ambientColor;
	float				_ambientIntensity;
	ToneMapperType		_toneMapperType;
	float				_exposure;
	// clang-format on
};