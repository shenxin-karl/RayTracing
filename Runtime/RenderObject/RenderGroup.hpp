#pragma once

struct RenderGroup {
	enum : uint16_t {
		eOpaque			= 1000,		
		eAlphaTest		= 2000,		
		eSkyBox			= 3000,		
		eTransparent	= 4000,
		ePostProcess	= 5000,
	};
public:
	static constexpr bool IsOpaque(uint16_t renderGroup) {
		return renderGroup <= eOpaque;
	}
	static constexpr bool IsAlphaTest(uint16_t renderGroup) {
		return renderGroup > eOpaque && renderGroup <= eAlphaTest;
	}
	static constexpr bool IsSkyBox(uint16_t renderGroup) {
		return renderGroup == eSkyBox;
	}
	static constexpr bool IsTransparent(uint16_t renderGroup) {
		return renderGroup > eSkyBox && renderGroup <= eTransparent;
	}
	static constexpr bool IsPostProcess(uint16_t renderGroup) {
		return renderGroup > eTransparent;
	}
};