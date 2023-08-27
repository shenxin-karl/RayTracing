#include "PathUtils.h"

namespace nstd {

bool IsSubPath(stdfs::path mainPath, stdfs::path subPath) {
	if (mainPath.empty() || subPath.empty()) {
		return false;
	}

	if (!mainPath.is_absolute()) {
		mainPath = stdfs::absolute(mainPath.lexically_normal());
	}
	if (!subPath.is_absolute()) {
		subPath = stdfs::absolute(subPath.lexically_normal());
	}
	if (mainPath == subPath) {
		return true;
	}

	std::string mainPathStr = mainPath.string();
	std::string subPathStr = subPath.string();
	return subPathStr.find(mainPathStr) != std::string::npos;
}

auto ToRelativePath(const stdfs::path &mainPath, const stdfs::path &subPath) -> std::optional<stdfs::path> {
	if (!IsSubPath(mainPath, subPath)) {
		return std::nullopt;
	}
	return std::make_optional(subPath.lexically_relative(mainPath));
}

}
