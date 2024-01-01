#include "UserMarker.h"
#include "D3d12/Context.h"
#include "Modules/Pix/Pix.h"
#include "WinPixEventRuntime/pix3.h"

UserMarker::UserMarker(const dx::Context *pContext, std::string_view name) : _pContext(pContext) {
    dx::NativeCommandList *pCmdList = _pContext->GetCommandList();
    if (Pix::IsLoaded()) {
        ::PIXBeginEvent(pCmdList, 0, name.data());
    } else {
        pCmdList->BeginEvent(1, name.data(), name.length() + 1);
    }
}

UserMarker::~UserMarker() {
    dx::NativeCommandList *pCmdList = _pContext->GetCommandList();
    if (Pix::IsLoaded()) {
        ::PIXEndEvent(_pContext->GetCommandList());
    } else {
        pCmdList->EndEvent();
    }
}
