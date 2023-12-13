#include "DDSLoader.h"
#include "Foundation/StreamUtil.h"
#include "D3d12/FormatHelper.hpp"
#include "Foundation/StreamUtil.h"

struct DDS_PIXELFORMAT {
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t bitCount;
    uint32_t bitMaskR;
    uint32_t bitMaskG;
    uint32_t bitMaskB;
    uint32_t bitMaskA;
};

struct DDS_HEADER {

    uint32_t dwSize;
    uint32_t dwHeaderFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t dwSurfaceFlags;
    uint32_t dwCubemapFlags;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};

enum RESOURCE_DIMENSION {
    RESOURCE_DIMENSION_UNKNOWN = 0,
    RESOURCE_DIMENSION_BUFFER = 1,
    RESOURCE_DIMENSION_TEXTURE1D = 2,
    RESOURCE_DIMENSION_TEXTURE2D = 3,
    RESOURCE_DIMENSION_TEXTURE3D = 4
};

struct DDS_HEADER_DXT10 {
    DXGI_FORMAT dxgiFormat;
    RESOURCE_DIMENSION resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t reserved;
};

static DXGI_FORMAT GetDxgiFormat(DDS_PIXELFORMAT pixelFmt) {
    if (pixelFmt.flags & 0x00000004)    //DDPF_FOURCC
    {
        // Check for D3DFORMAT enums being set here
        switch (pixelFmt.fourCC) {
        case '1TXD':
            return DXGI_FORMAT_BC1_UNORM;
        case '3TXD':
            return DXGI_FORMAT_BC2_UNORM;
        case '5TXD':
            return DXGI_FORMAT_BC3_UNORM;
        case 'U4CB':
            return DXGI_FORMAT_BC4_UNORM;
        case 'A4CB':
            return DXGI_FORMAT_BC4_SNORM;
        case '2ITA':
            return DXGI_FORMAT_BC5_UNORM;
        case 'S5CB':
            return DXGI_FORMAT_BC5_SNORM;
        case 'GBGR':
            return DXGI_FORMAT_R8G8_B8G8_UNORM;
        case 'BGRG':
            return DXGI_FORMAT_G8R8_G8B8_UNORM;
        case 36:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case 110:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case 111:
            return DXGI_FORMAT_R16_FLOAT;
        case 112:
            return DXGI_FORMAT_R16G16_FLOAT;
        case 113:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case 114:
            return DXGI_FORMAT_R32_FLOAT;
        case 115:
            return DXGI_FORMAT_R32G32_FLOAT;
        case 116:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    } else {
        switch (pixelFmt.bitMaskR) {
        case 0xff:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case 0x00ff0000:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case 0xffff:
            return DXGI_FORMAT_R16G16_UNORM;
        case 0x3ff:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case 0x7c00:
            return DXGI_FORMAT_B5G5R5A1_UNORM;
        case 0xf800:
            return DXGI_FORMAT_B5G6R5_UNORM;
        case 0:
            return DXGI_FORMAT_A8_UNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
        };
    }
}

bool ParseImageHead(const uint8_t *pHeaderData, dx::ImageHeader &outputImageHeader, size_t &inOutRawTextureSize) {
	const uint8_t *pByteData = pHeaderData;
    uint32_t dwMagic = *reinterpret_cast<const uint32_t *>(pByteData);
    if (dwMagic != ' SDD') {
        return false;
    }

    pByteData += 4;
    inOutRawTextureSize -= 4;

    inOutRawTextureSize -= sizeof(DDS_HEADER);
    const DDS_HEADER *header = reinterpret_cast<const DDS_HEADER *>(pByteData);
    outputImageHeader.width = header->dwWidth;
    outputImageHeader.height = header->dwHeight;
    outputImageHeader.depth = header->dwDepth ? header->dwDepth : 1;
    outputImageHeader.mipMapCount = header->dwMipMapCount ? header->dwMipMapCount : 1;

    if (header->ddspf.fourCC == '01XD') {
        DDS_HEADER_DXT10 *header10 = reinterpret_cast<DDS_HEADER_DXT10 *>((char *)header + sizeof(DDS_HEADER));
        inOutRawTextureSize -= sizeof(DDS_HEADER_DXT10);
        outputImageHeader.arraySize = header10->arraySize;
        outputImageHeader.format = header10->dxgiFormat;
        outputImageHeader.bitCount = header->ddspf.bitCount;
    } else {
        outputImageHeader.arraySize = (header->dwCubemapFlags == 0xfe00) ? 6 : 1;
        outputImageHeader.format = GetDxgiFormat(header->ddspf);
        outputImageHeader.bitCount = static_cast<uint32_t>(dx::BitsPerPixel(outputImageHeader.format));
    }
    return true;
}


FileDDSLoader::FileDDSLoader() : _imageHeader{} {
}

FileDDSLoader::~FileDDSLoader() {
    _file.close();
}

bool FileDDSLoader::Load(const stdfs::path &filePath, float cutOff) {
    if (!stdfs::exists(filePath)) {
        return false;
    }

    _file.open(filePath, std::ios::binary);
    if (!_file.is_open()) {
        return false;
    }

    size_t fileSize = nstd::GetFileSize(_file);
    size_t rawTextureSize = fileSize;

    uint8_t headerData[4 + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)];
    if (!_file.read(reinterpret_cast<char *>(headerData), sizeof(headerData))) {
        return false;
    }
	if (!ParseImageHead(headerData, _imageHeader, rawTextureSize)) {
		return false;
	}
    _file.seekg(fileSize - rawTextureSize, std::ios::beg);
    return true;
}

auto FileDDSLoader::GetImageHeader() const -> dx::ImageHeader {
    return _imageHeader;
}

void FileDDSLoader::GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) {
    for (uint32_t y = 0; y < height; y++) {
        _file.read(static_cast<char *>(pDest) + y * stride, width);
    }
}

MemoryDDSLoader::MemoryDDSLoader(): _pData(nullptr), _pCurrent(nullptr), _dataSize(0), _imageHeader{} {
}

MemoryDDSLoader::~MemoryDDSLoader() {
}

bool MemoryDDSLoader::Load(const uint8_t *pData, size_t dataSize, float cutOff) {
    if (_pData == nullptr || dataSize < sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)) {
	    return false;
    }

    _pData = pData;
    _pCurrent = _pData;
    _dataSize = dataSize;
    size_t rawTextureSize = _dataSize;
	if (!ParseImageHead(_pData, _imageHeader, rawTextureSize)) {
		return false;
	}
    _pCurrent = _pData + (_dataSize - rawTextureSize);
    return true;
}

auto MemoryDDSLoader::GetImageHeader() const -> dx::ImageHeader {
    return _imageHeader;
}

void MemoryDDSLoader::GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) {
    for (uint32_t y = 0; y < height; ++y) {
        size_t destOffset = y * stride;
	    std::memcpy(static_cast<char *>(pDest) + destOffset, _pCurrent, width);
        _pCurrent += width;
        Assert(_pCurrent <= _pData + _dataSize);
    }
}
