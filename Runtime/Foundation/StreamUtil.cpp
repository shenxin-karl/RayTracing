#include "StreamUtil.h"

namespace nstd {

std::streampos GetFileSize(std::ifstream &file) {
    std::streampos originalPos = file.tellg();
    file.seekg(0, std::ifstream::end);
    std::streampos fileSize = file.tellg();
    file.seekg(originalPos);
    return fileSize;
}

}    // namespace nstd
