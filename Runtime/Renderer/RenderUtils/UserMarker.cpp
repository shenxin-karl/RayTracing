#include "UserMarker.h"
#include "D3d12/Context.h"
#include "Modules/Pix/Pix.h"
#include "WinPixEventRuntime/pix3.h"

static UINT MakeColor(BYTE r, BYTE g, BYTE b) {
    return 0xff000000 | (r << 16) | (g << 8) | b;
}

UserMarker::UserMarker(const dx::Context *pContext, std::string_view name, glm::vec4 color) : _pContext(pContext) {
    dx::NativeCommandList *pCmdList = _pContext->GetCommandList();
    if (Pix::IsLoaded()) {
        UINT64 byteColor = MakeColor(static_cast<BYTE>(color.r * 255),
            static_cast<BYTE>(color.g * 255),
            static_cast<BYTE>(color.b * 255));
        ::PIXBeginEvent(pCmdList, byteColor, name.data());
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
