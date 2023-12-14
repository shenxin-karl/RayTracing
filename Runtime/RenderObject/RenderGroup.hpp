#pragma once

struct RenderGroup {
	enum {
		eOpaque			= 1000,
		eAlpha			= 2000,
		eSkyBox			= 3000,
		eTransparent	= 4000,
		ePostProcess	= 5000,
	};
};