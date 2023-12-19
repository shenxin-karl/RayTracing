#pragma once
#include "Renderer/RenderPasses/ForwardPass.h"

class StandardMaterialBatchDraw : public IMaterialBatchDraw {
public:
	void Draw(std::span<RenderObject * const> batch, const GlobalShaderParam &globalShaderParam) override;
};