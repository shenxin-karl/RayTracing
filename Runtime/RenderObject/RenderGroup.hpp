#pragma once

struct RenderGroup {
    enum : uint16_t {
        eOpaque = 1000,
        eAlphaTest = 2000,
        eSkyBox = 3000,
        eTransparent = 4000,
        ePostProcess = 5000,
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
    static constexpr bool IsSameGroup(uint16_t renderGroup1, uint16_t renderGroup2) {
        using TestFuncType = bool (*)(uint16_t);
        TestFuncType testFunc[5] = {
            &RenderGroup::IsOpaque,
            &RenderGroup::IsAlphaTest,
            &RenderGroup::IsSkyBox,
            &RenderGroup::IsTransparent,
            &RenderGroup::IsPostProcess,
        };
        for (auto f : testFunc) {
	        if (f(renderGroup1) && f(renderGroup2)) {
		        return true;
	        }
        }
        return false;
    }
};