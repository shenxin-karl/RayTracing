#pragma once
#include <unordered_set>
#include "Foundation/NamespeceAlias.h"
#include "Foundation/NonCopyable.h"

class ShaderDependency : NonCopyable {
public:
	ShaderDependency(const stdfs::path &path);
	auto GetLastWriteTime() const -> stdfs::file_time_type;
private:
	auto GetLastWriteTimeInternal(std::unordered_set<stdfs::path> &hashSet) const -> stdfs::file_time_type;
private:
	// clang-format off
	stdfs::path										_sourcePath;
	std::vector<stdfs::path>						_dependencies;
	mutable std::optional<stdfs::file_time_type>	_pLastWriteTime;
	// clang-format on
};