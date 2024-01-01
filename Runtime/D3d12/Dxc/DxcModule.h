#pragma once
#include "Foundation/Singleton.hpp"
#include "D3d12//D3dStd.h"

namespace dx {

class DxcModule : public Singleton<DxcModule> {
public:
    void OnCreate();
    void OnDestroy();
    auto GetCompiler3() const -> IDxcCompiler3 *;
    auto GetLinker() const -> IDxcLinker *;
    auto GetUtils() const -> IDxcUtils *;
    auto GetLibrary() const -> IDxcLibrary *;
private:
    WRL::ComPtr<IDxcUtils> _pUtils;
    WRL::ComPtr<IDxcLinker> _pLinker;
    WRL::ComPtr<IDxcLibrary> _pLibrary;
    WRL::ComPtr<IDxcCompiler3> _pCompiler;
};

}    // namespace dx
