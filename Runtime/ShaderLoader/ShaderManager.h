#pragma once
#include "Foundation/Singleton.hpp"
#include "Foundation/NamespeceAlias.h"
#include "Foundation/UUID128.h"
#include "D3d12/D3dUtils.h"

namespace dx {

class DefineList;
enum class ShaderType;

}    // namespace dx

// clang-format off
struct ShaderLoadInfo {
    stdfs::path             sourcePath;
    std::string_view        entryPoint;
    dx::ShaderType          shaderType;
    const dx::DefineList   *pDefineList = nullptr;
};
// clang-format on

class ShaderDependency;
class ShaderManager : public Singleton<ShaderManager> {
public:
    ShaderManager();
    ~ShaderManager() override;
public:
    void OnCreate();
    void OnDestroy();
    auto LoadShaderByteCode(const ShaderLoadInfo &loadInfo) -> D3D12_SHADER_BYTECODE;
private:
    friend class ShaderDependency;
    auto GetShaderDependency(stdfs::path path) -> ShaderDependency &;
    auto LoadFromCache(UUID128 uuid, const stdfs::path &sourcePath, const stdfs::path &cachePath)
        -> std::optional<D3D12_SHADER_BYTECODE>;

    using ShaderByteCodeMap = std::unordered_map<UUID128, std::vector<std::byte>>;
    using ShaderDependencyMap = std::unordered_map<stdfs::path, std::unique_ptr<ShaderDependency>>;
private:
    // clang-format off
    ShaderByteCodeMap   _shaderByteCodeMap;
    ShaderDependencyMap _shaderDependencyMap;
    // clang-format on
};