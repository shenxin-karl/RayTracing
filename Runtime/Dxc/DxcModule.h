#pragma once
#include "Foundation/Singleton.hpp"
#include "D3d12/D3dUtils.h"

class DxcModule : public Singleton<DxcModule> {
public:
    void OnCreate();
    void OnDestroy();
    auto GetCompiler3() const -> IDxcCompiler3 *;
    auto GetLinker() const -> IDxcLinker *;
    auto GetUtils() const -> IDxcUtils *;
    auto GetLibrary() const -> IDxcLibrary *;
private:
    Microsoft::WRL::ComPtr<IDxcUtils> _pUtils;
    Microsoft::WRL::ComPtr<IDxcLinker> _pLinker;
    Microsoft::WRL::ComPtr<IDxcLibrary> _pLibrary;
    Microsoft::WRL::ComPtr<IDxcCompiler3> _pCompiler;
};
