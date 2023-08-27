#pragma once
#include "Foundation/NamespeceAlias.h"

namespace nstd {

bool IsSubPath(stdfs::path mainPath, stdfs::path subPath);
auto ToRelativePath(const stdfs::path &mainPath, const stdfs::path &subPath) -> std::optional<stdfs::path>;


}