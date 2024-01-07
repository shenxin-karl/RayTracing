#include "UserMarker.h"
#include "D3d12/Context.h"
#include "Modules/Pix/Pix.h"
#include "WinPixEventRuntime/pix3.h"

UserMarker::UserMarker(const dx::Context *pContext, std::string_view name) {
    _pCommandList = pContext->GetCommandList();
    if (Pix::IsLoaded()) {
        ::PIXBeginEvent(_pCommandList, 0, name.data());
    } else {
        _pCommandList->BeginEvent(1, name.data(), name.length() + 1);
    }
}

UserMarker::UserMarker(ID3D12GraphicsCommandList *pCommandList, std::string_view name) {
    _pCommandList = pCommandList;
    if (Pix::IsLoaded()) {
        ::PIXBeginEvent(_pCommandList, 0, name.data());
    } else {
        _pCommandList->BeginEvent(1, name.data(), name.length() + 1);
    }
}

UserMarker::~UserMarker() {
    if (Pix::IsLoaded()) {
        ::PIXEndEvent(_pCommandList);
    } else {
        _pCommandList->EndEvent();
    }
}
