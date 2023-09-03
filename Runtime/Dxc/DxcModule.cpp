#include "DxcModule.h"

void DxcModule::OnCreate() {
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_pCompiler));
	DxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(&_pLinker));
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&_pUtils));
	DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&_pLibrary));
}

void DxcModule::OnDestroy() {
	_pUtils = nullptr;
	_pLinker = nullptr;
	_pLibrary = nullptr;
	_pCompiler = nullptr;
}

auto DxcModule::GetCompiler3() const -> IDxcCompiler3 * {
	return _pCompiler.Get();
}
auto DxcModule::GetLinker() const -> IDxcLinker * {
	return _pLinker.Get();
}
auto DxcModule::GetUtils() const -> IDxcUtils * {
	return _pUtils.Get();

}
auto DxcModule::GetLibrary() const -> IDxcLibrary * {
	return _pLibrary.Get();
}
