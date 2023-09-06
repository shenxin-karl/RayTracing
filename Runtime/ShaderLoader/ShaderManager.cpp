#include "ShaderManager.h"
#include "D3d12/ShaderCompiler.h"
#include "Foundation/PathUtils.h"
#include "Utils/AssetProjectSetting.h"
#include "Foundation/DebugBreak.h"
#include "Foundation/Logger.h"
#include "Foundation/StringUtil.h"
#include "ShaderDependency.h"

#include <fstream>
#include <magic_enum.hpp>

#include "Foundation/StreamUtil.h"

#if defined(MODE_DEBUG)
static std::string_view sShaderCacheDirectory = "Shader/Debug";
#elif defined(MODE_RELEASE)
static std::string_view sShaderCacheDirectory = "Shader/Release";
#elif defined(MODE_RELWITHDEBINFO)
static std::string_view sShaderCacheDirectory = "Shader/RelWithDebInfo";
#endif

ShaderManager::ShaderManager() {
}

ShaderManager::~ShaderManager() {
}

void ShaderManager::OnCreate() {
    stdfs::path shaderCacheDir = AssetProjectSetting::GetInstance()->GetAssetCacheAbsolutePath() /
                                 sShaderCacheDirectory;
    if (!stdfs::exists(shaderCacheDir)) {
        stdfs::create_directories(shaderCacheDir);
    }
    Exception::CondThrow(stdfs::is_directory(shaderCacheDir),
        "The cache path {} is occupied. Procedure",
        shaderCacheDir.string());
}

void ShaderManager::OnDestroy() {
    _shaderByteCodeMap.clear();
    _shaderDependencyMap.clear();
}

static bool ShaderIncludeCallBack(const std::string &path, std::string &fileContent) {
    stdfs::path assetAbsolutePath = AssetProjectSetting::GetInstance()->GetAssetAbsolutePath();
    std::optional<stdfs::path> pRelativePath = nstd::ToRelativePath(assetAbsolutePath, path);
    if (!pRelativePath) {
        DEBUG_BREAK;
        return false;
    }

    stdfs::path filePath = assetAbsolutePath / pRelativePath.value();
    std::fstream fin(filePath, std::ios::in);
    std::stringstream buffer;
    buffer << fin.rdbuf();
    fileContent = std::move(buffer).str();
    return true;
}

auto ShaderManager::LoadShaderByteCode(const ShaderLoadInfo &loadInfo) -> D3D12_SHADER_BYTECODE {
    stdfs::path sourcePath = loadInfo.sourcePath;
    if (!sourcePath.is_absolute()) {
        sourcePath = stdfs::absolute(sourcePath);
    }

    Exception::CondThrow(nstd::IsSubPath(AssetProjectSetting::GetInstance()->GetAssetAbsolutePath(), sourcePath),
        "Only shaders under the Asset path can be loaded");

    std::string keyString = fmt::format("{}_{}_{}_{}",
        sourcePath.string(),
        loadInfo.entryPoint.data(),
        magic_enum::enum_name(loadInfo.shaderType).data(),
        loadInfo.pDefineList != nullptr ? loadInfo.pDefineList->ToString() : "");

    UUID128 uuid = UUID128::New(keyString);
    if (auto iter = _shaderByteCodeMap.find(uuid); iter != _shaderByteCodeMap.end()) {
        const std::vector<std::byte> &byteCode = iter->second;
        return D3D12_SHADER_BYTECODE{byteCode.data(), byteCode.size()};
    }

    std::string cacheFileName = fmt::format("{}.cso", uuid.ToString());
    stdfs::path shaderCachePath = AssetProjectSetting::GetInstance()->GetAssetCacheAbsolutePath() /
                                  sShaderCacheDirectory / cacheFileName;
    if (std::optional<D3D12_SHADER_BYTECODE> pShaderByteCode = LoadFromCache(uuid, sourcePath, shaderCachePath)) {
        return pShaderByteCode.value();
    }

    dx::ShaderCompilerDesc desc = {sourcePath,
        loadInfo.entryPoint,
        loadInfo.shaderType,
        loadInfo.pDefineList,
        !CompileEnvInfo::IsModeRelWithDebInfo(),
        &ShaderIncludeCallBack};

    dx::ShaderCompiler shaderCompiler;
    if (!shaderCompiler.Compile(desc)) {
        Logger::Warning("Compile shader {} error: the error message: {}",
            sourcePath.string(),
            shaderCompiler.GetErrorMessage());
        DEBUG_BREAK;
        return {};
    }

    Microsoft::WRL::ComPtr<IDxcBlob> pShaderBlob = shaderCompiler.GetByteCode();
    std::ofstream fileOutput(shaderCachePath, std::ios::binary);
    fileOutput.write(static_cast<const char *>(pShaderBlob->GetBufferPointer()), pShaderBlob->GetBufferSize());
    fileOutput.close();

    std::vector<std::byte> byteCode;
    byteCode.resize(pShaderBlob->GetBufferSize());
    std::memcpy(byteCode.data(), pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize());
    _shaderByteCodeMap[uuid] = std::move(byteCode);

    return D3D12_SHADER_BYTECODE{pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize()};
}

auto ShaderManager::GetShaderDependency(stdfs::path path) -> ShaderDependency & {
    if (!path.is_absolute()) {
        path = stdfs::absolute(path);
    }

    auto iter = _shaderDependencyMap.find(path);
    if (iter != _shaderDependencyMap.end()) {
        return *iter->second;
    }

    auto item = std::make_pair(path, std::make_unique<ShaderDependency>(path));
    iter = _shaderDependencyMap.emplace_hint(iter, std::move(item));
    return *iter->second;
}

static D3D12_SHADER_BYTECODE CreateShaderByteCode(const std::vector<std::byte> &byteCode) {
    return D3D12_SHADER_BYTECODE{byteCode.data(), byteCode.size()};
}

auto ShaderManager::LoadFromCache(UUID128 uuid, const stdfs::path &sourcePath, const stdfs::path &cachePath)
    -> std::optional<D3D12_SHADER_BYTECODE> {

    if (!stdfs::exists(cachePath)) {
        return std::nullopt;
    }

    stdfs::file_time_type cacheLastWriteTime = stdfs::last_write_time(cachePath);
    ShaderDependency &dependency = GetShaderDependency(sourcePath);
    if (dependency.GetLastWriteTime() <= cacheLastWriteTime) {
        std::ifstream fin(cachePath, std::ios::binary);
		std::size_t fileSize = nstd::GetFileSize(fin);
        std::vector<std::byte> byteCode;
        byteCode.resize(fileSize);
        fin.read(reinterpret_cast<char *>(byteCode.data()), fileSize);
        std::vector<std::byte> &obj = _shaderByteCodeMap[uuid] = std::move(byteCode);
        return CreateShaderByteCode(obj);
    }
    return std::nullopt;
}
