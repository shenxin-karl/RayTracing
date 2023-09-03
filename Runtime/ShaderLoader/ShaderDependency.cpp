#include "ShaderDependency.h"
#include "ShaderManager.h"
#include "Foundation/Exception.h"
#include <fstream>
#include <regex>

ShaderDependency::ShaderDependency(const stdfs::path &path) {
    _sourcePath = stdfs::absolute(path);
    std::ifstream input(path);
    Exception::CondThrow(input.is_open(), "Can't open the file {}", path.string());

    std::string line;
    std::regex pattern("(?:^\\s*#include\\s*[\"<])([^\">]+)(?:[\">])(?:\\s*$)");
    while (std::getline(input, line)) {
        std::smatch match;
        if (std::regex_search(line, match, pattern)) {
            _dependencies.emplace_back(match[1].str());
        }
    }
}

auto ShaderDependency::GetLastWriteTime() const -> stdfs::file_time_type {
    std::unordered_set<stdfs::path> hashSet;
    return GetLastWriteTimeInternal(hashSet);
}

auto ShaderDependency::GetLastWriteTimeInternal(std::unordered_set<stdfs::path> &hashSet) const
    -> stdfs::file_time_type {

    if (_pLastWriteTime != std::nullopt) {
        return *_pLastWriteTime;
    }

    if (hashSet.contains(_sourcePath)) {
        return {};
    }
    hashSet.insert(_sourcePath);
    stdfs::file_time_type lastWriteTime = stdfs::last_write_time(_sourcePath);
    for (const auto &dependencyFile : _dependencies) {
        ShaderDependency &dependency = ShaderManager::GetInstance()->GetShaderDependency(dependencyFile);
        lastWriteTime = std::max(lastWriteTime, dependency.GetLastWriteTimeInternal(hashSet));
    }

    _pLastWriteTime = std::make_optional(lastWriteTime);
    return lastWriteTime;
}
