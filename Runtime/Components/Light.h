#pragma once
#include "Component.h"
#include <glm/glm.hpp>
#include <DirectXCollision.h>

class Light : public Component {
	DECLARE_CLASS(Light);
public:
	Light();
	~Light() override;
public:
	void SetColor(const glm::vec3 &color);
	void SetIntensity(float intensity);
	auto GetColor() const -> const glm::vec3 &;
	auto GetIntensity() const -> float;
private:
	// clang-format off
	glm::vec3	_color;
	float		_intensity;
	// clang-format on
};

class DirectionalLight : public Light {
	DECLARE_CLASS(DirectionalLight);
private:
	void OnAddToScene() override;
	void OnRemoveFormScene() override;
};