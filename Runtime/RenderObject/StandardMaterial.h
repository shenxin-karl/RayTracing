#pragma once
#include <memory>
#include "Foundation/NonCopyable.h"
#include "D3d12/DescriptorHandle.h"

namespace dx {
class Texture;
}

enum class RenderMode {
	eOpaque = 0,
	eAlphaTest = 1,
	eTransparent = 2,
};

class StandardMaterial : NonCopyable {
	enum TextureType {
		eAlbedo,
		eAmbientOcclusion,
		eEmission,
		eMetalRoughness,
		eNormal,
		eMaxNum
	};
public:
	StandardMaterial();
	~StandardMaterial();
private:
	RenderMode					 _renderMode;
	dx::SRV						 _textureHandles[eMaxNum];
	std::shared_ptr<dx::Texture> _textures[eMaxNum];
};