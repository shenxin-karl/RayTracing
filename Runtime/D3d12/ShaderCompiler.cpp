#include "ShaderCompiler.h"
#include "D3d12/Dxc/DxcModule.h"
#include "Foundation/PathUtils.h"
#include "Foundation/StringUtil.h"
#include <regex>

namespace dx {

void DefineList::Set(std::string_view key, int value) {
    iterator iter = Find(key);
    if (iter != _macroItems.end()) {
        iter->value = value;
    }
    auto &back = _macroItems.emplace_back();
    back.key = key;
    back.value = value;
}

auto DefineList::Get(std::string_view key) const -> std::optional<int> {
    iterator iter = Find(key);
    if (iter == _macroItems.end()) {
        return std::nullopt;
    }
    return std::make_optional(iter->value);
}

bool DefineList::Remove(std::string_view key) {
    iterator iter = Find(key);
    if (iter == _macroItems.end()) {
        return false;
    }
    _macroItems.erase(iter);
    return true;
}

auto DefineList::operator[](std::string_view key) -> int & {
    iterator iter = Find(key);
    if (iter != _macroItems.end()) {
        return iter->value;
    }
    auto &back = _macroItems.emplace_back();
    back.key = key;
    back.value = 1;
    return back.value;
}

auto DefineList::ToString() const -> std::string {
    std::ranges::stable_sort(_macroItems, [](const MacroItem &lhs, const MacroItem &rhs) { return lhs.key < rhs.key; });

    std::stringstream sbuf;
    for (const MacroItem &item : _macroItems) {
        sbuf << fmt::format("#{}={}", item.key.c_str(), item.value);
    }
    return std::move(sbuf).str();
}

static DefineList::MacroItem ParseMacroItem(std::string_view string) {
    DefineList::MacroItem item;
    size_t pos = string.find("=");
    Assert(pos != std::string_view::npos);
    item.key = string.substr(0, pos);
    item.value = std::stoi(string.substr(pos + 1).data());
    return item;
}

auto DefineList::FromString(std::string source) -> size_t {
    size_t count = 0;
    std::smatch match;
    std::regex pattern("[_a-zA-Z][a-zA-Z0-9_]*=[0-9]+");
    while (std::regex_search(source, match, pattern)) {
        _macroItems.push_back(ParseMacroItem(match.str()));
        ++count;
        source = match.suffix().str();
    }
    return count;
}

auto DefineList::Find(std::string_view key) const -> iterator {
    for (iterator iter = _macroItems.begin(); iter != _macroItems.end(); ++iter) {
        if (iter->key == key) {
            return iter;
        }
    }
    return _macroItems.end();
}

#pragma region ShaderCompiler

class CustomIncludeHandler : public IDxcIncludeHandler {
public:
    HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename,
        _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource) override {
        using Microsoft::WRL::ComPtr;
        ComPtr<IDxcBlobEncoding> pEncoding;

        std::string fileContent;
        std::string path = nstd::to_string(pFilename);
        if (!shaderIncludeCallback(path, fileContent)) {
            return S_FALSE;
        }

        HRESULT hr = DxcModule::GetInstance()->GetUtils()->CreateBlob(
            fileContent.data(),
            fileContent.length() + 1,
            DXC_CP_ACP,
            pEncoding.GetAddressOf());

        if (SUCCEEDED(hr)) {
            *ppIncludeSource = pEncoding.Detach();
            return S_OK;
        }
        return S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override {
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef(void) override {
        return 0;
    }
    ULONG STDMETHODCALLTYPE Release(void) override {
        return 0;
    }
public:
    ShaderIncludeCallback shaderIncludeCallback;
};

bool ShaderCompiler::Compile(const ShaderCompilerDesc &desc) {
    MainThread::EnsureMainThread();
    const stdfs::path &path = desc.path;
    const ShaderType type = desc.shaderType;
    const std::string_view entryPoint = desc.entryPoint;
    bool makeDebugInfo = desc.makeDebugInfo;
    const DefineList *pDefineList = desc.pDefineList;

    CustomIncludeHandler includeHandler;
    includeHandler.shaderIncludeCallback = desc.includeCallback;

    Microsoft::WRL::ComPtr<IDxcBlob> pSourceBlob;
    std::wstring fileName = nstd::to_wstring(path.string());
    _result = includeHandler.LoadSource(fileName.c_str(), pSourceBlob.GetAddressOf());
    if (FAILED(_result)) {
        _errorMessage = fmt::format("Can't open the file {}", path.string());
        return false;
    }

    std::wstring_view target;
    switch (type) {
    case ShaderType::eVS:
        target = L"vs_6_1";
        break;
    case ShaderType::eHS:
        target = L"hs_6_1";
        break;
    case ShaderType::eDS:
        target = L"ds_6_1";
        break;
    case ShaderType::eGS:
        target = L"gs_6_1";
        break;
    case ShaderType::ePS:
        target = L"ps_6_1";
        break;
    case ShaderType::eCS:
        target = L"cs_6_1";
        break;
    case ShaderType::eLib:
        target = L"lib_6_3";
        break;
    default:
        Exception::Throw("Error ShaderType");
        break;
    }

    std::wstring entryPointStr = nstd::to_wstring(entryPoint);
    std::vector<LPCWSTR> arguments = {fileName.c_str(), L"-T", target.data()};
    std::vector<std::wstring> macros;

    if (type != ShaderType::eLib) {
	    Assert(!entryPointStr.empty());
        arguments.push_back(L"-E");
    	arguments.push_back(entryPointStr.c_str());
    }

    if (makeDebugInfo) {
        arguments.push_back(L"-Zi");
        arguments.push_back(L"-Od");
        arguments.push_back(L"-Qembed_debug");
    }

    if (!desc.outputPDBPath.empty()) {
        macros.push_back(desc.outputPDBPath.wstring());
	    arguments.push_back(L"-Fd");
        arguments.push_back(macros.back().c_str());
    }

    if (pDefineList != nullptr) {
        for (auto &&[key, value] : *pDefineList) {
            std::string arg = fmt::format("-D{}={}", key, value);
            macros.push_back(nstd::to_wstring(arg));
            arguments.push_back(macros.back().c_str());
        }
    }

    // Compile shader
    DxcBuffer buffer{};
    buffer.Encoding = DXC_CP_ACP;
    buffer.Ptr = pSourceBlob->GetBufferPointer();
    buffer.Size = pSourceBlob->GetBufferSize();

    Microsoft::WRL::ComPtr<IDxcResult> pCompileResult;
    _result = DxcModule::GetInstance()->GetCompiler3()->Compile(&buffer,
        arguments.data(),
        static_cast<uint32_t>(arguments.size()),
        &includeHandler,
        IID_PPV_ARGS(&pCompileResult));

    if (pCompileResult == nullptr) {
        return false;
    }

    pCompileResult->GetStatus(&_result);
    if (FAILED(_result)) {
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> pErrorBlob;
        _result = pCompileResult->GetErrorBuffer(&pErrorBlob);
        _errorMessage = static_cast<const char *>(pErrorBlob->GetBufferPointer());
        return false;
    }

    _result = pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&_pByteCode), nullptr);
    bool ret = SUCCEEDED(_result);

    if (!desc.outputPDBPath.empty()) {
		_result = pCompileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&_pPDB), nullptr);
        ret = ret && SUCCEEDED(_result);
    }
    return ret;
}

auto ShaderCompiler::GetErrorMessage() const -> const std::string & {
    return _errorMessage;
}

auto ShaderCompiler::GetByteCode() const -> WRL::ComPtr<IDxcBlob> {
    return _pByteCode;
}

auto ShaderCompiler::GetPDB() const -> WRL::ComPtr<IDxcBlob> {
    return _pPDB;
}

#pragma endregion

}    // namespace dx
